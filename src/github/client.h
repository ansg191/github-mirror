//
// Created by Anshul Gupta on 4/4/25.
//

#ifndef GITHUB_CLIENT_H
#define GITHUB_CLIENT_H

#include "../client.h"

#include "types.h"

char *github_identity(const gql_client *client);

int github_list_user_repos(const gql_client *client, const char *username,
			   const char *after, struct gh_list_repos_res *res);

#endif // GITHUB_CLIENT_H
