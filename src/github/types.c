//
// Created by Anshul Gupta on 4/4/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "types.h"


char *identity_from_json(const cJSON *root)
{
	cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
	if (!data || !cJSON_IsObject(data)) {
		fprintf(stderr, "Error: data object not found\n");
		return NULL;
	}
	cJSON *viewer = cJSON_GetObjectItemCaseSensitive(data, "viewer");
	if (!viewer || !cJSON_IsObject(viewer)) {
		fprintf(stderr, "Error: viewer object not found\n");
		return NULL;
	}
	cJSON *login = cJSON_GetObjectItemCaseSensitive(viewer, "login");
	if (!login || !cJSON_IsString(login)) {
		fprintf(stderr, "Error: login not found\n");
		return NULL;
	}

	return strdup(login->valuestring);
}

int gh_list_repos_from_json(cJSON *root, struct gh_list_repos_res *res)
{
	int status = 0;
	cJSON *repo;

	// Initialize the response structure
	memset(res, 0, sizeof(*res));

	// Get the data object
	cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
	if (!data || !cJSON_IsObject(data)) {
		fprintf(stderr, "Error: data object not found\n");
		status = -1;
		goto end;
	}

	// Get the repositoryOwner object
	cJSON *repositoryOwner = cJSON_GetObjectItemCaseSensitive(
			data, "repositoryOwner");
	if (!repositoryOwner || !cJSON_IsObject(repositoryOwner)) {
		fprintf(stderr, "Error: repositoryOwner object not found\n");
		status = -1;
		goto end;
	}

	// Get the repositories object
	cJSON *repositories = cJSON_GetObjectItemCaseSensitive(repositoryOwner,
							       "repositories");
	if (!repositories || !cJSON_IsObject(repositories)) {
		fprintf(stderr, "Error: repositories object not found\n");
		status = -1;
		goto end;
	}

	// Get the pageInfo object
	cJSON *page_info = cJSON_GetObjectItemCaseSensitive(repositories,
							    "pageInfo");
	if (!page_info || !cJSON_IsObject(page_info)) {
		fprintf(stderr, "Error: pageInfo object not found\n");
		status = -1;
		goto end;
	}

	// Get the hasNextPage and endCursor values
	cJSON *has_next_page = cJSON_GetObjectItemCaseSensitive(page_info,
								"hasNextPage");
	if (!has_next_page || !cJSON_IsBool(has_next_page)) {
		fprintf(stderr, "Error: hasNextPage not found\n");
		status = -1;
		goto end;
	}
	res->has_next_page = cJSON_IsTrue(has_next_page);

	cJSON *end_cursor = cJSON_GetObjectItemCaseSensitive(page_info,
							     "endCursor");
	if (!end_cursor || !cJSON_IsString(end_cursor)) {
		fprintf(stderr, "Error: endCursor not found\n");
		status = -1;
		goto end;
	}
	res->end_cursor = strdup(end_cursor->valuestring);

	// Get the nodes array
	cJSON *nodes = cJSON_GetObjectItemCaseSensitive(repositories, "nodes");
	if (!nodes || !cJSON_IsArray(nodes)) {
		fprintf(stderr, "Error: nodes array not found\n");
		status = -1;
		goto end;
	}

	// Iterate over the nodes array
	size_t len = cJSON_GetArraySize(nodes);
	res->repos = malloc(sizeof(*res->repos) * len);
	cJSON_ArrayForEach(repo, nodes)
	{
		cJSON *name = cJSON_GetObjectItemCaseSensitive(repo, "name");
		if (!name || !cJSON_IsString(name)) {
			fprintf(stderr, "Error: name not found\n");
			status = -1;
			goto end;
		}
		cJSON *url = cJSON_GetObjectItemCaseSensitive(repo, "url");
		if (!url || !cJSON_IsString(url)) {
			fprintf(stderr, "Error: url not found\n");
			status = -1;
			goto end;
		}
		cJSON *ssh_url_v = cJSON_GetObjectItemCaseSensitive(repo,
								    "sshUrl");
		if (!ssh_url_v || !cJSON_IsString(ssh_url_v)) {
			fprintf(stderr, "Error: sshUrl not found\n");
			status = -1;
			goto end;
		}
		const char *prefix = "ssh://";
		char *ssh_url = malloc(strlen(prefix) +
				       strlen(ssh_url_v->valuestring) + 1);
		if (!ssh_url) {
			fprintf(stderr, "Error: malloc failed\n");
			status = -1;
			goto end;
		}
		strcpy(ssh_url, prefix);
		strcat(ssh_url, ssh_url_v->valuestring);
		// Replace 2nd colon with slash
		char *colon = strchr(ssh_url + strlen(prefix), ':');
		if (colon)
			*colon = '/';


		cJSON *is_fork = cJSON_GetObjectItemCaseSensitive(repo,
								  "isFork");
		if (!is_fork || !cJSON_IsBool(is_fork)) {
			fprintf(stderr, "Error: isFork not found\n");
			status = -1;
			goto end;
		}
		cJSON *is_private = cJSON_GetObjectItemCaseSensitive(
				repo, "isPrivate");
		if (!is_private || !cJSON_IsBool(is_private)) {
			fprintf(stderr, "Error: isPrivate not found\n");
			status = -1;
			goto end;
		}

		res->repos[res->repos_len].name = strdup(name->valuestring);
		res->repos[res->repos_len].url = strdup(url->valuestring);
		res->repos[res->repos_len].ssh_url = ssh_url;
		res->repos[res->repos_len].is_fork = cJSON_IsTrue(is_fork);
		res->repos[res->repos_len].is_private =
				cJSON_IsTrue(is_private);
		res->repos_len++;
	}

end:
	cJSON_Delete(root);
	if (status != 0)
		gh_list_repos_res_free(*res);
	return status;
}

void gh_list_repos_res_free(struct gh_list_repos_res res)
{
	free(res.end_cursor);
	for (size_t i = 0; i < res.repos_len; i++) {
		free(res.repos[i].name);
		free(res.repos[i].url);
	}
	free(res.repos);
}
