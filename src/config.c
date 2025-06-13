//
// Created by Anshul Gupta on 4/6/25.
//

#include "config.h"

#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

const char *config_locations[] = {
		"config.ini",
		"github-mirror.conf",
		"/usr/local/github-mirror/config.ini",
		"/usr/local/github-mirror.conf",
		"/usr/local/etc/github-mirror/config.ini",
		"/usr/local/etc/github-mirror.conf",
		"/etc/github-mirror/config.ini",
		"/etc/github-mirror.conf",
		NULL,
};

enum config_section {
	section_none,
	section_github,
	section_srht,
	section_git,
};


static char *file_read(const char *path, size_t *size_out)
{
	char *contents = NULL;

	// Open the file for reading
	const int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Error reading config file");
		return NULL;
	}

	// Stat the file to get its size
	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("Error getting file size");
		goto end;
	}

	// Get the size of the file
	const size_t size = st.st_size;
	if (size == 0) {
		fprintf(stderr, "Error reading config file: file is empty\n");
		goto end;
	}

	// Map the file into memory
	contents = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (contents == MAP_FAILED) {
		perror("Error reading config file");
		goto end;
	}

	// Output file size
	if (size_out)
		*size_out = size;

end:
	close(fd);
	return contents;
}

static char *trim(char *start, char *end)
{
	while (start < end && isspace(*start))
		start++;
	while (end > start && isspace(*(end - 1)))
		end--;
	*end = '\0';
	return start;
}

/**
 * Parse a token value from the config file.
 * If the token is a file path, read the file and set the token to its contents.
 * If not a file path, the raw token value is checked for validity.
 * If it is a github token, it then checks that the token starts with:
 *	- "ghp_"
 *	- "gho_"
 *	- "ghu_"
 *	- "ghs_"
 *	- "ghf_"
 *	- "github_pat_"
 * @param value token value
 * @return Owned token value or NULL on error
 */
static char *parse_token(char *value, enum remote_type tp)
{
	char *token = NULL;

	// Open the file to check if it is a file
	const int fd = open(value, O_RDONLY);
	if (fd == -1) {
		if (errno != ENOENT) {
			perror("Error opening token file");
			return NULL;
		}
		// Not a file, token check
		token = strdup(value);
		goto token_check;
	}

	// Read the file
	token = malloc(1024);
	if (!token) {
		perror("Error allocating token buffer");
		close(fd);
		return NULL;
	}

	// Read the file contents
	const ssize_t bytes_read = read(fd, token, 1024);
	if (bytes_read < 0) {
		perror("Error reading token file");
		free(token);
		close(fd);
		return NULL;
	}
	token[bytes_read] = '\0';

	// Trim whitespace
	char *tmp = strdup(trim(token, token + bytes_read));
	free(token);
	if (!tmp) {
		perror("Error allocating token buffer");
		close(fd);
		return NULL;
	}
	token = tmp;

	close(fd);

token_check:
	if (tp == remote_type_srht)
		return token;
	if (!strncmp(token, "ghp_", 4) || !strncmp(token, "gho_", 4) ||
	    !strncmp(token, "ghu_", 4) || !strncmp(token, "ghs_", 4) ||
	    !strncmp(token, "ghf_", 4) || !strncmp(token, "github_pat_", 11)) {
		return token;
	}
	fprintf(stderr, "Error: invalid token format: %s\n", token);
	free(token);
	return NULL;
}

static int parse_bool(const char *value, int *out)
{
	if (!strcmp(value, "true") || !strcmp(value, "1") ||
	    !strcmp(value, "yes") || !strcmp(value, "on")) {
		*out = 1;
		return 0;
	}
	if (!strcmp(value, "false") || !strcmp(value, "0") ||
	    !strcmp(value, "no") || !strcmp(value, "off")) {
		*out = 0;
		return 0;
	}
	return -1;
}

static int parse_line_inner(struct config *cfg, enum config_section section,
			    char *key, char *value)
{
	switch (section) {
	case section_none:
		fprintf(stderr,
			"Unexpected key-value pair outside of section: %s=%s\n",
			key, value);
		return -1;
	case section_github:
		if (!strcmp(key, "endpoint"))
			cfg->head->gh.endpoint = value;
		else if (!strcmp(key, "token")) {
			cfg->head->gh.token =
					parse_token(value, cfg->head->type);
		} else if (!strcmp(key, "user_agent"))
			cfg->head->gh.user_agent = value;
		else if (!strcmp(key, "owner"))
			cfg->head->gh.owner = value;
		else if (!strcmp(key, "skip-forks")) {
			if (parse_bool(value, &cfg->head->gh.skip_forks) < 0) {
				fprintf(stderr,
					"Error parsing config file: "
					"invalid value for skip-forks: %s\n",
					value);
				return -1;
			}
		} else if (!strcmp(key, "skip-private")) {
			if (parse_bool(value, &cfg->head->gh.skip_private) <
			    0) {
				fprintf(stderr,
					"Error parsing config file: "
					"invalid value for skip-private: %s\n",
					value);
				return -1;
			}
		} else if (!strcmp(key, "transport")) {
			if (!strcmp(value, "ssh"))
				cfg->head->gh.transport = git_transport_ssh;
			else if (!strcmp(value, "https"))
				cfg->head->gh.transport = git_transport_https;
			else {
				fprintf(stderr,
					"Error parsing config file: "
					"invalid value for transport: %s\n",
					value);
				return -1;
			}
		} else {
			fprintf(stderr,
				"Error parsing config file: unknown key: %s\n",
				key);
			return -1;
		}
		break;
	case section_srht:
		if (!strcmp(key, "endpoint"))
			cfg->head->srht.endpoint = value;
		else if (!strcmp(key, "token")) {
			cfg->head->srht.token =
					parse_token(value, cfg->head->type);
		} else if (!strcmp(key, "user_agent")) {
			cfg->head->srht.user_agent = value;
		} else if (!strcmp(key, "owner")) {
			cfg->head->srht.owner = value;
		} else {
			fprintf(stderr,
				"Error parsing config file: unknown key: %s\n",
				key);
			return -1;
		}
		break;
	case section_git:
		if (!strcmp(key, "base"))
			cfg->git_base = value;
		else {
			fprintf(stderr,
				"Error parsing config file: unknown key: %s\n",
				key);
			return -1;
		}
		break;
	}
	return 0;
}

static int parse_line(struct config *cfg, char *line,
		      enum config_section *section)
{
	switch (*line) {
	case ';':
	case '#':
	case '\0':
		// Ignore comments and empty lines
		return 0;
	case '[': {
		// Handle section headers
		char *close = strchr(line, ']');
		if (!close) {
			fprintf(stderr,
				"Error parsing config file: invalid section "
				"header: %s\n",
				line);
			return -1;
		}
		*close = '\0';
		char *section_name = trim(line + 1, close);
		if (!strcmp(section_name, "github")) {
			*section = section_github;

			// Add the new remote to the list
			struct remote_cfg *remote = calloc(1, sizeof(*remote));
			if (!remote) {
				perror("Error allocating owner");
				return -1;
			}
			remote->type = remote_type_github;
			remote->gh.endpoint = GH_DEFAULT_ENDPOINT;
			remote->gh.user_agent = DEFAULT_USER_AGENT;
			remote->gh.transport = git_transport_https;
			remote->next = cfg->head;
			cfg->head = remote;
		} else if (!strcmp(section_name, "srht")) {
			*section = section_srht;

			// Add the new remote to the list
			struct remote_cfg *remote = calloc(1, sizeof(*remote));
			if (!remote) {
				perror("Error allocating owner");
				return -1;
			}
			remote->type = remote_type_srht;
			remote->srht.endpoint = SRHT_DEFAULT_ENDPOINT;
			remote->srht.user_agent = DEFAULT_USER_AGENT;
			remote->next = cfg->head;
			cfg->head = remote;
		} else if (!strcmp(section_name, "git"))
			*section = section_git;
		else {
			fprintf(stderr,
				"Error parsing config file: unknown section: "
				"%s\n",
				section_name);
			return -1;
		}
		return 0;
	}
	default: {
		// Handle key-value pairs
		char *line_end = line + strlen(line);
		char *equals = strchr(line, '=');
		if (!equals) {
			fprintf(stderr,
				"Error parsing config file: invalid line: %s\n",
				line);
			return -1;
		}
		*equals = '\0';
		char *key = trim(line, equals);
		char *value = trim(equals + 1, line_end);
		return parse_line_inner(cfg, *section, key, value);
	}
	}
}

static int config_parse(struct config *cfg)
{
	char *ptr = cfg->contents;
	char *end = cfg->contents + cfg->contents_len;
	enum config_section section = section_none;

	while (ptr < end) {
		// Find the end of the line
		char *newline = memchr(ptr, '\n', end - ptr);
		char *line_end = newline ? newline : end;

		// Handle line endings
		char *actual_end = line_end;
		if (actual_end > ptr && *(actual_end - 1) == '\r')
			actual_end--;

		// Null-terminate the line
		if (line_end < end)
			*line_end = '\0';

		// Trim whitespace
		char *line = trim(ptr, actual_end);

		// Parse the line
		if (parse_line(cfg, line, &section) < 0)
			return -1;

		// Move to the next line
		ptr = newline ? newline + 1 : end;
	}

	return 0;
}

static void config_defaults(struct config *cfg)
{
	cfg->quiet = 0;
	cfg->git_base = "/srv/git";
}

static int github_cfg_validate(const struct github_cfg *cfg)
{
	if (!cfg->token) {
		fprintf(stderr, "Error: missing required field: "
				"github.token\n");
		return -1;
	}
	if (!cfg->owner) {
		fprintf(stderr, "Error: missing required field: "
				"github.owner\n");
		return -1;
	}
	return 0;
}

static int srht_cfg_validate(const struct srht_cfg *cfg)
{
	if (!cfg->token) {
		fprintf(stderr, "Error: missing required field: "
				"srht.token\n");
		return -1;
	}

	if (!cfg->owner) {
		fprintf(stderr, "Error: missing required field: "
				"srht.owner\n");
		return -1;
	}
	return 0;
}

static int config_validate(const struct config *cfg)
{
	const struct remote_cfg *remote = cfg->head;
	while (remote) {
		switch (remote->type) {
		case remote_type_github:
			if (github_cfg_validate(&remote->gh) < 0)
				return -1;
			break;
		case remote_type_srht:
			if (srht_cfg_validate(&remote->srht) < 0)
				return -1;
			break;
		}
		remote = remote->next;
	}
	return 0;
}


struct config *config_read(const char *path)
{
	struct config *cfg = calloc(1, sizeof(*cfg));
	if (!cfg) {
		perror("error allocating config");
		return NULL;
	}
	config_defaults(cfg);

	// Read the config file
	cfg->contents = file_read(path, &cfg->contents_len);
	if (!cfg->contents)
		goto fail;

	// Parse the config file
	if (config_parse(cfg) < 0)
		goto fail2;

	// Validate the config file
	if (config_validate(cfg) < 0)
		goto fail2;

	return cfg;

fail2:
	munmap(cfg->contents, cfg->contents_len);
fail:
	free(cfg);
	return NULL;
}

static void remote_cfg_free(struct remote_cfg *remote)
{
	switch (remote->type) {
	case remote_type_github:
		free((char *) remote->gh.token);
		break;
	case remote_type_srht:
		free((char *) remote->srht.token);
		break;
	}
}

void config_free(struct config *config)
{
	if (!config || !config->contents)
		return;
	munmap(config->contents, config->contents_len);

	// Free the remote_cfg list
	struct remote_cfg *owner = config->head;
	while (owner) {
		struct remote_cfg *next = owner->next;
		remote_cfg_free(owner);
		free(owner);
		owner = next;
	}

	free(config);
}
