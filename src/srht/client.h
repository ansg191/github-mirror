//
// Created by Anshul Gupta on 6/10/25.
//

#ifndef SRHT_CLIENT_H
#define SRHT_CLIENT_H

#include "../client.h"

#include "types.h"

int srht_list_user_repos(const gql_client *client, const char *username,
			 const char *cursor, struct srht_list_repos_res *res);

#endif // SRHT_CLIENT_H
