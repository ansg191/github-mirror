//
// Created by Anshul Gupta on 4/9/25.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>

#include "../src/config.h"

static void config_read_empty(void **state)
{
	(void) state;
	const char *path = "../tests/fixtures/empty.ini";
	struct config *cfg = config_read(path);
	assert_null(cfg);
}

static void config_read_normal(void **state)
{
	(void) state;
	const char *path = "../tests/fixtures/normal.ini";
	struct config *cfg = config_read(path);
	assert_non_null(cfg);
	assert_string_equal(cfg->endpoint, "https://api.github.com/graphql");
	assert_string_equal(cfg->token, "token");
	assert_string_equal(cfg->user_agent, "user-agent");
	assert_string_equal(cfg->owner, "my-org");
	assert_string_equal(cfg->git_base, "/srv/git");
	assert_int_equal(cfg->git_owner, 1000);
	assert_int_equal(cfg->git_group, 1001);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(config_read_empty),
		cmocka_unit_test(config_read_normal)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
