//
// Created by Anshul Gupta on 4/6/25.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>

#define GH_DEFAULT_ENDPOINT "https://api.github.com/graphql"
#define GH_DEFAULT_USER_AGENT ("github_mirror/" GITHUB_MIRROR_VERSION)

extern const char *config_locations[];

struct github_cfg {
	/// Whether to skip mirroring fork repositories
	int skip_forks;
	/// Whether to skip mirroring private repositories
	int skip_private;

	// Borrowed
	/// Github graphql API endpoint
	const char *endpoint;
	/// Client user agent
	const char *user_agent;
	/// The owner of the repositories
	const char *owner;

	// Owned
	/// Github auth token
	const char *token;
	/// Next owner in the list
	struct github_cfg *next;
};

struct config {
	/// The content of the config file
	char *contents;
	size_t contents_len;

	/// Quiet mode
	int quiet;

	/// Repo owners to mirror
	struct github_cfg *head;

	/// The filepath to the git mirrors
	/// Default: /srv/git
	const char *git_base;
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
