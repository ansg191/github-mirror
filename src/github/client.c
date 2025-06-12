//
// Created by Anshul Gupta on 4/4/25.
//

#include "client.h"

#include <stdlib.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

#include "queries/github/gh_identity.h"
#include "queries/github/gh_list_repos.h"

#include "../buffer.h"
#include "types.h"

char *github_identity(const gql_client *client)
{
	char *login = NULL;
	buffer_t buf = buffer_new(4096);

	const CURLcode ret = gql_client_send(client, gh_identity, NULL, &buf);
	if (ret != CURLE_OK) {
		fprintf(stderr, "Failed to send request: %s\n",
			curl_easy_strerror(ret));
		goto fail;
	}

	// Parse the response
	cJSON *root = cJSON_Parse((const char *) buf.data);
	if (!root) {
		const char *err = cJSON_GetErrorPtr();
		if (err)
			fprintf(stderr, "Error parsing response: %s\n", err);
		goto fail;
	}

	// Check for errors
	if (gql_handle_error(root) < 0)
		goto fail1;

	// Get login from json
	login = identity_from_json(root);

fail1:
	cJSON_Delete(root);
fail:
	buffer_free(buf);
	return login;
}

int github_list_user_repos(const gql_client *client, const char *username,
			   const char *after, struct gh_list_repos_res *res)
{
	int status = 0;
	buffer_t buf = buffer_new(4096);

	cJSON *args = cJSON_CreateObject();
	if (!args) {
		status = -1;
		goto end;
	}
	cJSON_AddItemToObject(args, "username", cJSON_CreateString(username));
	cJSON_AddItemToObject(args, "after", cJSON_CreateString(after));

	const CURLcode ret = gql_client_send(client, gh_list_repos, args, &buf);
	if (ret != CURLE_OK) {
		fprintf(stderr, "Failed to send request: %s\n",
			curl_easy_strerror(ret));
		status = -1;
		goto end;
	}

	// Parse the response
	cJSON *root = cJSON_Parse((const char *) buf.data);
	if (!root) {
		const char *err = cJSON_GetErrorPtr();
		if (err)
			fprintf(stderr, "Error parsing response: %s\n", err);
		status = -1;
		goto end;
	}

	// Check for errors
	if (gql_handle_error(root) < 0) {
		status = -1;
		goto end;
	}

	// Convert json to struct
	if (gh_list_repos_from_json(root, res) < 0) {
		fprintf(stderr, "Failed to parse response\n");
		status = -1;
	}

end:
	buffer_free(buf);
	return status;
}
