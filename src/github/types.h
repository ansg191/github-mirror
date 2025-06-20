//
// Created by Anshul Gupta on 4/4/25.
//

#ifndef GITHUB_TYPES_H
#define GITHUB_TYPES_H

#include <cjson/cJSON.h>

char *identity_from_json(const cJSON *root);

struct gh_list_repos_res {
	int has_next_page;
	char *end_cursor;

	struct {
		char *name;
		char *url;
		char *ssh_url;
		int is_fork;
		int is_private;
	} *repos;

	size_t repos_len;
};

int gh_list_repos_from_json(cJSON *root, struct gh_list_repos_res *res);
void gh_list_repos_res_free(struct gh_list_repos_res res);

#endif // GITHUB_TYPES_H
