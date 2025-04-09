//
// Created by Anshul Gupta on 4/7/25.
//

#include "precheck.h"

#include <stdio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/capability.h>
#endif

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
 * Check if we have CAP_CHOWN capability or if we are root.
 * @return 1 if we have chown capability, 0 if not.
 */
static int has_chown(void)
{
#ifdef __linux__
	cap_t caps = cap_get_proc();
	if (caps == NULL) {
		perror("cap_get_proc");
		return 0;
	}

	cap_flag_value_t cap_value;
	if (cap_get_flag(caps, CAP_CHOWN, CAP_EFFECTIVE, &cap_value) == -1) {
		perror("cap_get_flag");
		cap_free(caps);
		return 0;
	}
	cap_free(caps);

	if (cap_value == CAP_SET)
		return 1; // We have CAP_CHOWN capability
	fprintf(stderr, "Error: CAP_CHOWN capability is not set\n");
	return 0; // We don't have CAP_CHOWN capability
#else
	if (geteuid() == 0)
		return 1;
	fprintf(stderr, "Error: not running as root\n");
	return 0;
#endif
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
	if (!has_chown())
		return -1;
	if (!git_base_exists(cfg->git_base))
		return -1;
	return 0;
}
