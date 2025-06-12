//
// Created by Anshul Gupta on 6/10/25.
//

#ifndef SRHT_TYPES_H
#define SRHT_TYPES_H

#include <cjson/cJSON.h>

struct srht_list_repos_res {
	char *cursor;
	char *canonical_name;

	struct {
		char *name;
		char *url;
	} *repos;
	size_t repos_len;
};

int srht_list_repos_from_json(cJSON *root, struct srht_list_repos_res *res);
void srht_list_repos_res_free(struct srht_list_repos_res res);

#endif // SRHT_TYPES_H
