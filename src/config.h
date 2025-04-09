//
// Created by Anshul Gupta on 4/6/25.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <sys/types.h>

#define GH_DEFAULT_ENDPOINT "https://api.github.com/graphql"
#define GH_DEFAULT_USER_AGENT "github_mirror/0.1"

extern const char *config_locations[];

struct config {
	/// The content of the config file
	char *contents;
	size_t contents_len;

	const char *endpoint;
	const char *token;
	const char *user_agent;

	/// The owner of the repositories
	const char *owner;

	/// The filepath to the git mirrors
	/// Default: /srv/git
	const char *git_base;
	/// User to give ownership of the git mirrors
	uid_t git_owner;
	/// Group to give ownership of the git mirrors
	gid_t git_group;
};

/**
 * Read the INI config file at the given path.
 * Will print to stderr errors if the file cannot be read.
 * @param path Path to the config file
 * @return A pointer to the config struct, or NULL on error
 */
struct config *config_read(const char *path);

/**
 * Free the config struct
 * @param config The config struct to free
 */
void config_free(struct config *config);

#endif // CONFIG_H
