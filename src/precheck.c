//
// Created by Anshul Gupta on 4/7/25.
//

#include "precheck.h"

#include <stdio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Check if git is installed and available in the PATH.
 * @return 1 if git is available, 0 if not.
 */
static int has_git(void)
{
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return 0;
	}

	if (pid == 0) {
		// Child process
		char *args[] = {"git", "--version", NULL};
		execvp("git", args);
		_exit(127); // execvp only returns on error
	}

	// Parent process
	int status;
	pid_t result;
	do {
		result = waitpid(pid, &status, 0);
	} while (result == -1 && errno == EINTR);
	if (result == -1) {
		perror("waitpid");
		return 0;
	}

	// Check exit status
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		return 1; // git is available
	}

	fprintf(stderr, "Error: git is not installed or not found in PATH\n");
	return 0; // git is not available
}

/**
 * Check if the git base directory exists.
 * @param path Path to the directory to check
 * @return 1 if the directory exists, 0 if not.
 */
static int git_base_exists(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
		return 1;
	}
	fprintf(stderr, "Error: git base directory does not exist: %s\n", path);
	return 0;
}

int precheck_self(const struct config *cfg)
{
	if (!has_git())
		return -1;
	if (!git_base_exists(cfg->git_base))
		return -1;
	return 0;
}
