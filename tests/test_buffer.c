//
// Created by Anshul Gupta on 4/9/25.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>

#include "../src/buffer.h"

static void buffer_new_empty_test(void **state)
{
	(void) state;
	const buffer_t buf = buffer_new(0);
	assert_null(buf.data);
	assert_int_equal(buf.len, 0);
	assert_int_equal(buf.cap, 0);
}

static void buffer_new_test(void **state)
{
	(void) state;
	const buffer_t buf = buffer_new(10);
	assert_non_null(buf.data);
	assert_int_equal(buf.len, 0);
	assert_int_equal(buf.cap, 16); // 10 aligned to 8 bytes
	buffer_free(buf);
}

static void buffer_reserve_test(void **state)
{
	(void) state;
	buffer_t buf = buffer_new(10);
	assert_int_equal(buf.cap, 16); // 10 aligned to 8 bytes
	buffer_reserve(&buf, 20);
	assert_int_equal(buf.cap, 24); // 20 aligned to 8 bytes
	buffer_free(buf);
}

static void buffer_append_test(void **state)
{
	(void) state;
	buffer_t buf = buffer_new(0);
	assert_int_equal(buf.cap, 0);
	const char *data = "Hello, World!";
	const size_t len = strlen(data);
	buffer_append(&buf, data, len);
	assert_int_equal(buf.len, len);
	assert_int_equal(buf.cap, 16); // 16 bytes allocated
	assert_memory_equal(buf.data, data, len);
	buffer_append(&buf, data, len);
	assert_int_equal(buf.len, len * 2);
	assert_int_equal(buf.cap, 32); // 32 bytes allocated
	assert_memory_equal(buf.data + len, data, len);
	buffer_free(buf);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(buffer_new_empty_test),
		cmocka_unit_test(buffer_new_test),
		cmocka_unit_test(buffer_reserve_test),
		cmocka_unit_test(buffer_append_test),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
