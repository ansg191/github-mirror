#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

#include "client.h"
#include "config.h"
#include "git.h"
#include "github/client.h"
#include "github/types.h"
#include "precheck.h"
#include "srht/client.h"
#include "srht/types.h"

static int load_config(int argc, char **argv, struct config **cfg_out)
{
	int opt, opt_idx = 0;
	size_t i;
	int quiet = 0;
	char *cfg_path = NULL;

	static struct option long_options[] = {
			{"version", no_argument, 0, 'v'},
			{"config", required_argument, 0, 'c'},
			{"help", no_argument, 0, 'h'},
			{"quiet", no_argument, 0, 'q'},
			{0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "C:c:h:q:v", long_options,
				  &opt_idx)) != -1) {
		switch (opt) {
		case 'C':
		case 'c':
			cfg_path = optarg;
			break;
		case 'h':
			fprintf(stderr,
				"Usage: %s [--config <file>] [--help]\n",
				argv[0]);
			return 0;
		case 'q':
			quiet = 1;
			break;
		case 'v':
			fprintf(stderr, "github-mirror v%s\n",
				GITHUB_MIRROR_VERSION);
			return 0;
		default:
			fprintf(stderr, "Unknown option: %c\n", opt);
			fprintf(stderr,
				"Usage: %s [--config <file>] [--quiet] "
				"[--help]\n",
				argv[0]);
			return 1;
		}
	}

	// Config file given, use it
	if (cfg_path) {
		*cfg_out = config_read(cfg_path);
		if (*cfg_out) {
			(*cfg_out)->quiet = quiet;
			return 0;
		}
		return 1;
	}

	// No config file given, try the default locations
	for (i = 0; config_locations[i]; i++) {
		if (!quiet)
			fprintf(stderr, "Trying config file: %s\n",
				config_locations[i]);
		*cfg_out = config_read(config_locations[i]);
		if (*cfg_out) {
			if (!quiet)
				fprintf(stderr, "Using config file: %s\n",
					config_locations[i]);
			(*cfg_out)->quiet = quiet;
			return 0;
		}
	}
	fprintf(stderr, "Failed to read config file\n");
	return 1;
}

static int mirror_github(const char *git_base, const struct github_cfg *cfg,
			 int quiet)
{
	if (!quiet)
		printf("Mirroring Github owner: %s\n", cfg->owner);

	const struct gql_ctx ctx = {
			.endpoint = cfg->endpoint,
			.token = cfg->token,
			.user_agent = cfg->user_agent,
	};
	gql_client *client = gql_client_new(ctx);
	if (!client) {
		fprintf(stderr, "Failed to create GitHub client\n");
		return 1;
	}

	// Get identity
	char *login = github_identity(client);

	struct gh_list_repos_res res;
	char *end_cursor = NULL;
	int status = 0;
	do {
		if (github_list_user_repos(client, cfg->owner, end_cursor,
					   &res))
			return -1;

		for (size_t i = 0; i < res.repos_len; i++) {
			if (cfg->skip_forks && res.repos[i].is_fork) {
				if (!quiet)
					printf("Skipping forked repo: %s\n",
					       res.repos[i].name);
				continue;
			}
			if (cfg->skip_private && res.repos[i].is_private) {
				if (!quiet)
					printf("Skipping private repo: %s\n",
					       res.repos[i].name);
				continue;
			}

			const char *url = cfg->transport == git_transport_ssh
							  ? res.repos[i].ssh_url
							  : res.repos[i].url;

			if (!quiet)
				printf("Repo: %s\t%s\n", res.repos[i].name,
				       url);

			const struct repo_ctx repo = {
					.git_base = git_base,
					.owner = cfg->owner,
					.token = cfg->token,
					.name = res.repos[i].name,
					.url = url,
					.username = login,
			};
			if (git_mirror_repo(&repo, quiet) != 0) {
				fprintf(stderr, "Failed to mirror repo\n");
				status = -1;
				break;
			}
		}

		free(end_cursor);
		end_cursor = strdup(res.end_cursor);

		gh_list_repos_res_free(res);
	} while (res.has_next_page);

	free(end_cursor);
	free(login);
	gql_client_free(client);
	return status;
}

static int mirror_srht(const char *git_base, const struct srht_cfg *cfg,
		       int quiet)
{
	if (!quiet)
		printf("Mirroring sr.ht owner: %s\n", cfg->owner);

	const struct gql_ctx ctx = {
			.endpoint = cfg->endpoint,
			.token = cfg->token,
			.user_agent = cfg->user_agent,
	};
	gql_client *client = gql_client_new(ctx);
	if (!client) {
		fprintf(stderr, "Failed to create sr.ht client\n");
		return 1;
	}

	struct srht_list_repos_res res;
	char *cursor = NULL;
	int status = 0;
	do {
		if (srht_list_user_repos(client, cfg->owner, cursor, &res))
			return -1;

		for (size_t i = 0; i < res.repos_len; i++) {
			if (!quiet)
				printf("Repo: %s\t%s\n", res.repos[i].name,
				       res.repos[i].url);

			const struct repo_ctx repo = {
					.git_base = git_base,
					.owner = res.canonical_name,
					.token = cfg->token,
					.name = res.repos[i].name,
					.url = res.repos[i].url,
					.username = res.canonical_name,
			};
			if (git_mirror_repo(&repo, quiet) != 0) {
				fprintf(stderr, "Failed to mirror repo\n");
				status = -1;
				break;
			}
		}

		if (res.cursor) {
			free(cursor);
			cursor = strdup(res.cursor);
		}
		srht_list_repos_res_free(res);
	} while (res.cursor != NULL);

	free(cursor);
	gql_client_free(client);
	return status;
}


int main(int argc, char **argv)
{
	setbuf(stdout, NULL);
	curl_global_init(CURL_GLOBAL_DEFAULT);

	struct config *cfg = NULL;
	const int ret = load_config(argc, argv, &cfg);
	if (ret != 0 || !cfg)
		return ret;

	if (precheck_self(cfg)) {
		fprintf(stderr, "Precheck failed\n");
		config_free(cfg);
		return 1;
	}

	int status = 0;
	const struct remote_cfg *remote = cfg->head;
	while (remote) {
		switch (remote->type) {
		case remote_type_github:
			if (mirror_github(cfg->git_base, &remote->gh,
					  cfg->quiet)) {
				fprintf(stderr, "Failed to mirror owner: %s\n",
					remote->gh.owner);
				status = 1;
			}
			break;
		case remote_type_srht:
			if (mirror_srht(cfg->git_base, &remote->srht,
					cfg->quiet)) {
				fprintf(stderr,
					"Failed to mirror sr.ht owner: %s\n",
					remote->srht.owner);
				status = 1;
			}
			break;
		}
		remote = remote->next;
	}

	config_free(cfg);
	curl_global_cleanup();
	return status;
}
