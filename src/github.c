//
// Created by Anshul Gupta on 4/4/25.
//

#include "github.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include <curl/curl.h>

#include "queries/identity.h"
#include "queries/list_repos.h"

#include "buffer.h"
#include "github_types.h"

struct gh_impl {
	struct github_ctx ctx;
	CURL *curl;
};

static CURLcode gh_impl_send(const struct gh_impl *client, const char *query,
			     cJSON *args, buffer_t *buf);

static int gh_handle_error(const cJSON *root);

github_client *github_client_new(const struct github_ctx ctx)
{
	struct gh_impl *c = malloc(sizeof(*c));
	if (!c)
		return NULL;

	c->ctx.endpoint = strdup(ctx.endpoint);
	c->ctx.token = strdup(ctx.token);
	c->ctx.user_agent = strdup(ctx.user_agent);
	c->curl = curl_easy_init();
	return c;
}

github_client *github_client_dup(github_client *client)
{
	struct gh_impl *c = client;
	if (!c)
		return NULL;

	struct gh_impl *dup = malloc(sizeof(*dup));
	if (!dup)
		return NULL;

	dup->ctx.endpoint = strdup(c->ctx.endpoint);
	dup->ctx.token = strdup(c->ctx.token);
	dup->ctx.user_agent = strdup(c->ctx.user_agent);
	dup->curl = curl_easy_duphandle(c->curl);

	return dup;
}

void github_client_free(github_client *client)
{
	struct gh_impl *c = client;
	if (!c)
		return;
	curl_easy_cleanup(c->curl);
	free((char *) c->ctx.endpoint);
	free((char *) c->ctx.token);
	free((char *) c->ctx.user_agent);
	free(c);
}

char *github_client_identity(const github_client *client)
{
	char *login = NULL;
	const struct gh_impl *c = client;
	buffer_t buf = buffer_new(4096);

	const CURLcode ret = gh_impl_send(c, identity, NULL, &buf);
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
	if (gh_handle_error(root) < 0)
		goto fail1;

	// Get login from json
	login = identity_from_json(root);

fail1:
	cJSON_Delete(root);
fail:
	buffer_free(buf);
	return login;
}

int github_client_list_user_repos(const github_client *client,
				  const char *username, const char *after,
				  struct list_repos_res *res)
{
	int status = 0;
	const struct gh_impl *c = client;
	buffer_t buf = buffer_new(4096);

	cJSON *args = cJSON_CreateObject();
	if (!args) {
		status = -1;
		goto end;
	}
	cJSON_AddItemToObject(args, "username", cJSON_CreateString(username));
	cJSON_AddItemToObject(args, "after", cJSON_CreateString(after));

	const CURLcode ret = gh_impl_send(c, list_repos, args, &buf);
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
	if (gh_handle_error(root) < 0) {
		status = -1;
		goto end;
	}

	// Convert json to struct
	if (list_repos_from_json(root, res) < 0) {
		fprintf(stderr, "Failed to parse response\n");
		status = -1;
	}

end:
	buffer_free(buf);
	return status;
}

/**
 * Wraps the query in a JSON object with a "query" key and an optional
 * "variables" key
 * @param query GraphQL query
 * @param args GraphQL arguments
 * @return JSON string
 */
static char *wrap_query(const char *query, cJSON *args)
{
	cJSON *root = NULL, *query_str = NULL;
	char *str = NULL;

	root = cJSON_CreateObject();
	if (!root)
		return NULL;

	query_str = cJSON_CreateString(query);
	if (!query_str)
		goto end;

	// Transfer ownership of the string to root
	cJSON_AddItemToObject(root, "query", query_str);

	// Add the args object if it exists
	if (args)
		cJSON_AddItemToObject(root, "variables", args);

	str = cJSON_Print(root);
end:
	cJSON_Delete(root);
	return str;
}

static size_t write_data(const void *ptr, const size_t size, size_t nmemb,
			 void *stream)
{
	(void) size; // unused

	buffer_t *buf = stream;
	buffer_append(buf, ptr, nmemb);
	return nmemb;
}

/**
 * Send a GraphQL query to the GitHub API
 * @param client Github Client
 * @param query GraphQL query
 * @return CURLcode
 */
static CURLcode gh_impl_send(const struct gh_impl *client, const char *query,
			     cJSON *args, buffer_t *buf)
{
	struct curl_slist *headers = NULL;
	char auth[1024];

	// Set the URL
	curl_easy_setopt(client->curl, CURLOPT_URL, client->ctx.endpoint);

	// Set the authorization header
	snprintf(auth, sizeof(auth), "Authorization: bearer %s",
		 client->ctx.token);
	headers = curl_slist_append(headers, auth);
	// Set the content type to JSON
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);

	// Set user agent
	curl_easy_setopt(client->curl, CURLOPT_USERAGENT,
			 client->ctx.user_agent);

	// Set the request type to POST
	curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "POST");

	// Prepare request body
	char *wrapped_query = wrap_query(query, args);

	// Set the request body
	curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, wrapped_query);
	curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE,
			 strlen(wrapped_query));

	// Set the write function to capture the response
	curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, (void *) buf);

	// Perform the request
	const CURLcode ret = curl_easy_perform(client->curl);

	// Append null terminator to the buffer
	if (ret == CURLE_OK)
		buffer_append(buf, "\0", 1);

	// Cleanup
	free(wrapped_query);
	curl_slist_free_all(headers);

	return ret;
}

/**
 * Handle errors in the response.
 * Will check for the "errors" key in the response and print the error messages.
 * Returns failure if any errors are found.
 * @param root Parsed JSON response
 * @return 0 on success (no errors), -1 on failure
 */
static int gh_handle_error(const cJSON *root)
{
	// Check for errors
	cJSON *errors = cJSON_GetObjectItemCaseSensitive(root, "errors");
	if (!errors || !cJSON_IsArray(errors)) {
		// No errors
		return 0;
	}

	cJSON *err;
	cJSON_ArrayForEach(err, errors)
	{
		// Get the error message
		cJSON *message = cJSON_GetObjectItemCaseSensitive(err,
								  "message");
		if (message && cJSON_IsString(message)) {
			fprintf(stderr, "Github Error: %s\n",
				message->valuestring);
		} else {
			fprintf(stderr, "Github Error: Unknown error\n");
		}
	}

	return -1;
}
