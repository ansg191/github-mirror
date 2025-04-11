//
// Created by Anshul Gupta on 4/6/25.
//

#include "git.h"

#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

/**
 * Constructs the full path to the git repository based on the base path, owner,
 * and name. If the name is NULL, it constructs the path to the owner's
 * directory.
 * @param base Base path for the git repository
 * @param owner Owner of the repository
 * @param name Name of the repository
 * @return A string containing the full path to the git repository or owner's
 * directory.
 */
static char *get_git_path(const char *base, const char *owner, const char *name)
{
	if (!base || !owner)
		return NULL;

	const char *format = name ? "%s/%s/%s.git" : "%s/%s";

	// Calculate the length of the string
	const int len = snprintf(NULL, 0, format, base, owner, name);
	if (len < 0)
		return NULL;

	// Allocate memory for the string
	char *path = malloc(len + 1);
	if (!path)
		return NULL;

	// Format the string
	if (snprintf(path, len + 1, format, base, owner, name) < 0) {
		free(path);
		return NULL;
	}
	return path;
}

/**
 * Adds authentication information to the HTTPS URL for git.
 * @param url The HTTPS URL to modify
 * @param user The username for authentication
 * @param token The token for authentication
 * @return A new string containing the modified URL with authentication
 * information
 */
static char *add_url_auth(const char *url, const char *user, const char *token)
{
	const char *https_prefix = "https://";
	const size_t prefix_len = strlen(https_prefix);
	size_t new_len;
	char *new_url;

	if (!url || !user || !token)
		return NULL;

	// Find the position of "https://"
	if (strncmp(url, https_prefix, prefix_len) != 0) {
		fprintf(stderr, "Error: URL does not start with https://\n");
		return NULL;
	}

	// Calculate the length of the new URL
	new_len = strlen(url) + strlen(user) + strlen(token) +
		  2; // 2 for "@" and ":"

	// Allocate memory for the new URL
	if (!((new_url = malloc(new_len + 1)))) {
		perror("malloc");
		return NULL;
	}

	// Construct the new URL
	snprintf(new_url, new_len + 1, "https://%s:%s@%s", user, token,
		 url + prefix_len);

	return new_url;
}

/**
 * Checks if the git repository at the specified path is a mirror.
 * @param path Path to the git repository
 * @return 1 if the repository is a mirror, 0 if not
 */
static int contains_mirror(const char *path)
{
	const pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return 0;
	}

	if (pid == 0) {
		// Child process

		// Redirect stdout to /dev/null
		const int devnull = open("/dev/null", O_WRONLY);
		if (devnull == -1) {
			perror("open");
			_exit(127);
		}
		if (dup2(devnull, STDOUT_FILENO) == -1) {
			perror("dup2");
			close(devnull);
			_exit(127);
		}
		close(devnull);

		// Run git
		char *args[] = {
				"git",	  "--git-dir", (char *) path,
				"config", "--get",     "remote.origin.mirror",
				NULL,
		};
		execvp("git", args);
		perror("execvp");
		_exit(127); // execvp only returns on error
	}

	int status;
	pid_t result;
	while ((result = waitpid(pid, &status, 0)) == -1 && errno == EINTR) {
	}
	if (result == -1) {
		perror("waitpid");
		return 0;
	}
	if (WIFEXITED(status)) {
		// Check if the exit status is 0 (success)
		if (WEXITSTATUS(status) == 0)
			return 1; // Repo exists
		if (WEXITSTATUS(status) == 1)
			return 0; // Repo does not exist
	}
	fprintf(stderr, "Error: git command failed with status %d\n",
		WEXITSTATUS(status));
	return 0; // Error occurred
}

/**
 * Creates a mirror of the git repository at the specified path.
 * @param path Full path to the git repository
 * @param ctx Context containing the repository information
 * @param quiet Suppress output if non-zero
 * @return 0 on success, -1 on error
 */
static int create_mirror(const char *path, const struct repo_ctx *ctx,
			 const int quiet)
{
	const pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {
		// Child process
		// Convert the URL to a format that git can use
		char *url = add_url_auth(ctx->url, ctx->username,
					 ctx->cfg->token);

		// Run git
		char *args[7];
		int i = 0;
		args[i++] = "git";
		args[i++] = "clone";
		args[i++] = "--mirror";
		if (quiet)
			args[i++] = "--quiet";
		args[i++] = url;
		args[i++] = (char *) path;
		args[i] = NULL;

		execvp("git", args);
		perror("execvp");
		_exit(127); // execvp only returns on error
	}

	int status;
	pid_t result;
	while ((result = waitpid(pid, &status, 0)) == -1 && errno == EINTR) {
	}
	if (result == -1) {
		perror("waitpid");
		return -1;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return 0; // Success
	fprintf(stderr, "Error: git clone failed with status %d\n",
		WEXITSTATUS(status));
	return -1; // Error occurred
}

/**
 * Creates the directory structure for the git repository.
 * @param ctx Repository context
 * @return 0 on success, -1 on error
 */
static int create_git_path(const struct repo_ctx *ctx)
{
	// Create owner directory if it doesn't exist
	char *owner_path = get_git_path(ctx->git_base, ctx->cfg->owner, NULL);
	if (!owner_path)
		return -1;
	if (mkdir(owner_path, 0755) == -1 && errno != EEXIST) {
		perror("mkdir");
		free(owner_path);
		return -1;
	}
	free(owner_path);

	// Create repo directory if it doesn't exist
	char *repo_path =
			get_git_path(ctx->git_base, ctx->cfg->owner, ctx->name);
	if (!repo_path)
		return -1;
	if (mkdir(repo_path, 0755) == -1 && errno != EEXIST) {
		perror("mkdir");
		free(repo_path);
		return -1;
	}
	free(repo_path);
	return 0;
}

/**
 * Updates the git repository at the specified path from the remote.
 * @param path Full path to the git repository
 * @param quiet Suppress output if non-zero
 * @return 0 on success, -1 on error
 */
static int update_mirror(const char *path, int quiet)
{
	const pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {
		// Child process
		char *args[9];
		int i = 0;
		args[i++] = "git";
		args[i++] = "--git-dir";
		args[i++] = (char *) path;
		args[i++] = "fetch";
		args[i++] = "--prune";
		if (quiet)
			args[i++] = "--quiet";
		args[i++] = "--all";
		args[i++] = "--tags";
		args[i] = NULL;

		execvp("git", args);
		perror("execvp");
		_exit(127); // execvp only returns on error
	}

	int status;
	pid_t result;
	while ((result = waitpid(pid, &status, 0)) == -1 && errno == EINTR) {
	}
	if (result == -1) {
		perror("waitpid");
		return -1;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return 0; // Success
	fprintf(stderr, "Error: git remote update failed with status %d\n",
		WEXITSTATUS(status));
	return -1; // Error occurred
}

int git_mirror_repo(const struct repo_ctx *ctx, int quiet)
{
	int ret = 0;
	char *path = get_git_path(ctx->git_base, ctx->cfg->owner, ctx->name);
	if (!path) {
		perror("get_git_path");
		return -1;
	}

	// Check whether repo exists
	if (contains_mirror(path)) {
		// Repo exists, so we can just update it
		if (!quiet)
			printf("Repo already exists, updating...\n");
		if (update_mirror(path, quiet) == -1) {
			perror("update_mirror");
			ret = -1;
			goto end;
		}
		goto end;
	}

	// Repo does not exist, so we need to clone it
	if (!quiet)
		printf("Repo does not exist, cloning...\n");
	if (create_git_path(ctx) == -1) {
		perror("create_git_path");
		ret = -1;
		goto end;
	}
	if (create_mirror(path, ctx, quiet) == -1) {
		perror("create_mirror");
		ret = -1;
	}

end:
	free(path);
	return ret;
}
