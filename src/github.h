//
// Created by Anshul Gupta on 4/4/25.
//

#ifndef GITHUB_H
#define GITHUB_H

#include "github_types.h"

typedef void github_client;

struct github_ctx {
	const char *endpoint;
	const char *token;
	const char *user_agent;
};

github_client *github_client_new(struct github_ctx ctx);

github_client *github_client_dup(github_client *client);

void github_client_free(github_client *client);

char *github_client_identity(const github_client *client);

int github_client_list_user_repos(const github_client *client,
				  const char *username, const char *after,
				  struct list_repos_res *res);

#endif // GITHUB_H
