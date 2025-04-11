//
// Created by Anshul Gupta on 4/6/25.
//

#ifndef GIT_H
#define GIT_H

#include "config.h"

struct repo_ctx {
	const char *git_base;
	const struct github_cfg *cfg;

	/// Name of the repo
	const char *name;
	/// HTTPS URL of the repo
	const char *url;
	/// GitHub username for authentication
	const char *username;
};

int git_mirror_repo(const struct repo_ctx *ctx);


#endif // GIT_H
