//
// Created by Anshul Gupta on 6/10/25.
//

#include "client.h"

#include <stdlib.h>
#include <string.h>

struct gql_impl {
	struct gql_ctx ctx;
	CURL *curl;
};

gql_client *gql_client_new(struct gql_ctx ctx)
{
	struct gql_impl *c = malloc(sizeof(*c));
	if (!c)
		return NULL;

	c->ctx.endpoint = strdup(ctx.endpoint);
	c->ctx.token = strdup(ctx.token);
	c->ctx.user_agent = strdup(ctx.user_agent);
	c->curl = curl_easy_init();
	return c;
}

gql_client *gql_client_dup(gql_client *client)
{
	struct gql_impl *c = client;
	if (!c)
		return NULL;

	struct gql_impl *dup = malloc(sizeof(*dup));
	if (!dup)
		return NULL;

	dup->ctx.endpoint = strdup(c->ctx.endpoint);
	dup->ctx.token = strdup(c->ctx.token);
	dup->ctx.user_agent = strdup(c->ctx.user_agent);
	dup->curl = curl_easy_duphandle(c->curl);
	return dup;
}

void gql_client_free(gql_client *client)
{
	struct gql_impl *c = client;
	if (!c)
		return;
	curl_easy_cleanup(c->curl);
	free((char *) c->ctx.endpoint);
	free((char *) c->ctx.token);
	free((char *) c->ctx.user_agent);
	free(c);
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

CURLcode gql_client_send(const gql_client *client, const char *query,
			 cJSON *args, buffer_t *buf)
{
	struct gql_impl *c = (struct gql_impl *) client;
	struct curl_slist *headers = NULL;
	char auth[1024];

	// Set the URL
	curl_easy_setopt(c->curl, CURLOPT_URL, c->ctx.endpoint);

	// Set the authorization header
	snprintf(auth, sizeof(auth), "Authorization: Bearer %s", c->ctx.token);
	headers = curl_slist_append(headers, auth);
	// Set the content type to JSON
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, headers);

	// Set user agent
	curl_easy_setopt(c->curl, CURLOPT_USERAGENT, c->ctx.user_agent);

	// Set the request type to POST
	curl_easy_setopt(c->curl, CURLOPT_CUSTOMREQUEST, "POST");

	// Prepare request body
	char *wrapped_query = wrap_query(query, args);

	// Set the request body
	curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, wrapped_query);
	curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, strlen(wrapped_query));

	// Set the write function to capture the response
	curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, (void *) buf);

	// Perform the request
	const CURLcode ret = curl_easy_perform(c->curl);

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
int gql_handle_error(const cJSON *root)
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
