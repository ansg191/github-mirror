//
// Created by Anshul Gupta on 6/10/25.
//

#include "client.h"

#include "queries/srht/srht_list_repos.h"

#include "../buffer.h"

int srht_list_user_repos(const gql_client *client, const char *username,
			 const char *cursor, struct srht_list_repos_res *res)
{
	int status = 0;
	buffer_t buf = buffer_new(4096);

	cJSON *args = cJSON_CreateObject();
	if (!args) {
		status = -1;
		goto end;
	}
	cJSON_AddItemToObject(args, "username", cJSON_CreateString(username));

	if (cursor != NULL)
		cJSON_AddItemToObject(args, "cursor",
				      cJSON_CreateString(cursor));
	else
		cJSON_AddItemToObject(args, "cursor", cJSON_CreateNull());

	const CURLcode ret =
			gql_client_send(client, srht_list_repos, args, &buf);
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
		cJSON_Delete(root);
		status = -1;
		goto end;
	}

	// Convert json to struct
	if (srht_list_repos_from_json(root, res) < 0) {
		fprintf(stderr, "Failed to parse response\n");
		status = -1;
	}

end:
	buffer_free(buf);
	return status;
}
