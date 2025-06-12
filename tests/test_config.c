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
	assert_string_equal(cfg->git_base, "/srv/git");

	assert_non_null(cfg->head);
	assert_int_equal(cfg->head->type, remote_type_github);
	assert_string_equal(cfg->head->gh.endpoint,
			    "https://api.github.com/graphql");
	assert_string_equal(cfg->head->gh.token, "ghp_1234567890abcdef");
	assert_string_equal(cfg->head->gh.user_agent, "user-agent");
	assert_string_equal(cfg->head->gh.owner, "my-org");
	config_free(cfg);
}

static void config_read_srht(void **state)
{
	(void) state;
	const char *path = "../tests/fixtures/srht.ini";
	struct config *cfg = config_read(path);
	assert_non_null(cfg);
	assert_string_equal(cfg->git_base, "/srv/git");

	assert_non_null(cfg->head);
	assert_int_equal(cfg->head->type, remote_type_srht);
	assert_string_equal(cfg->head->srht.endpoint,
			    "https://git.sr.ht/query");
	assert_string_equal(cfg->head->srht.token, "ABC123XYZ");
	assert_string_equal(cfg->head->srht.user_agent, "user-agent");
	assert_string_equal(cfg->head->srht.owner, "my-org");
	config_free(cfg);
}

int main(void)
{
	const struct CMUnitTest tests[] = {cmocka_unit_test(config_read_empty),
					   cmocka_unit_test(config_read_normal),
					   cmocka_unit_test(config_read_srht)};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
