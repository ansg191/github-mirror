//
// Created by Anshul Gupta on 6/10/25.
//

#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SRHT_GIT_BASE_URL "ssh://git@git.sr.ht/"

static char *srht_url(const char *canonical_name, const char *repo_name)
{
	if (!canonical_name || !repo_name) {
		fprintf(stderr, "Error: canonical_name or repo_name is NULL\n");
		return NULL;
	}

	size_t len = strlen(SRHT_GIT_BASE_URL) + strlen(canonical_name) +
		     strlen(repo_name) + 2; // +2 for '/' and '\0'
	char *url = malloc(len);
	if (!url) {
		fprintf(stderr, "Error: memory allocation failed for url\n");
		return NULL;
	}
	snprintf(url, len, "%s%s/%s", SRHT_GIT_BASE_URL, canonical_name,
		 repo_name);
	return url;
}

int srht_list_repos_from_json(cJSON *root, struct srht_list_repos_res *res)
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

	// Get the user object
	cJSON *user = cJSON_GetObjectItemCaseSensitive(data, "user");
	if (!user || !cJSON_IsObject(user)) {
		fprintf(stderr, "Error: user object not found\n");
		status = -1;
		goto end;
	}

	// Get the canonicalName value
	cJSON *canonical_name =
			cJSON_GetObjectItemCaseSensitive(user, "canonicalName");
	if (!canonical_name || !cJSON_IsString(canonical_name)) {
		fprintf(stderr, "Error: canonicalName not found\n");
		status = -1;
		goto end;
	}
	res->canonical_name = strdup(canonical_name->valuestring);

	// Get the repositories object
	cJSON *repositories =
			cJSON_GetObjectItemCaseSensitive(user, "repositories");
	if (!repositories || !cJSON_IsObject(repositories)) {
		fprintf(stderr, "Error: repositories object not found\n");
		status = -1;
		goto end;
	}

	// Get the cursor value
	cJSON *cursor = cJSON_GetObjectItemCaseSensitive(repositories,
							 "cursor");
	if (!cursor) {
		fprintf(stderr, "Error: cursor not found\n");
		status = -1;
		goto end;
	}
	if (cJSON_IsNull(cursor))
		res->cursor = NULL;
	else if (cJSON_IsString(cursor))
		res->cursor = strdup(cursor->valuestring);
	else {
		fprintf(stderr, "Error: cursor is not a string or null\n");
		status = -1;
		goto end;
	}

	// Get the results array
	cJSON *results = cJSON_GetObjectItemCaseSensitive(repositories,
							  "results");
	if (!results || !cJSON_IsArray(results)) {
		fprintf(stderr, "Error: results array not found\n");
		status = -1;
		goto end;
	}

	// Iterate over the results array
	size_t len = cJSON_GetArraySize(results);
	res->repos = malloc(sizeof(*res->repos) * len);
	if (!res->repos) {
		fprintf(stderr, "Error: memory allocation failed for repos\n");
		status = -1;
		goto end;
	}
	cJSON_ArrayForEach(repo, results)
	{
		if (!cJSON_IsObject(repo)) {
			fprintf(stderr,
				"Error: expected an object in results array\n");
			status = -1;
			goto end;
		}

		cJSON *name = cJSON_GetObjectItemCaseSensitive(repo, "name");
		if (!name || !cJSON_IsString(name)) {
			fprintf(stderr,
				"Error: name not found in repo object\n");
			status = -1;
			goto end;
		}
		res->repos[res->repos_len].name = strdup(name->valuestring);
		res->repos[res->repos_len].url =
				srht_url(res->canonical_name,
					 res->repos[res->repos_len].name);
		res->repos_len++;
	}

end:
	cJSON_Delete(root);
	if (status != 0)
		srht_list_repos_res_free(*res);
	return status;
}

void srht_list_repos_res_free(struct srht_list_repos_res res)
{
	free(res.cursor);
	free(res.canonical_name);
	for (size_t i = 0; i < res.repos_len; i++) {
		free(res.repos[i].name);
		free(res.repos[i].url);
	}
	free(res.repos);
}
