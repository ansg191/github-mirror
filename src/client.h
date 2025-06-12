//
// Created by Anshul Gupta on 6/10/25.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <cjson/cJSON.h>
#include <curl/curl.h>

#include "buffer.h"

typedef void gql_client;

struct gql_ctx {
	const char *endpoint;
	const char *token;
	const char *user_agent;
};

gql_client *gql_client_new(struct gql_ctx ctx);
gql_client *gql_client_dup(gql_client *client);
void gql_client_free(gql_client *client);

CURLcode gql_client_send(const gql_client *client, const char *query,
			 cJSON *args, buffer_t *buf);

int gql_handle_error(const cJSON *root);

#endif // CLIENT_H
