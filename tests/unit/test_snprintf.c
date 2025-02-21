// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <stdio.h>
#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "sizeof.h"
#include "string/sprintf/snprintf.h"


static void test_stprintf_a_trunc(void **);
static void test_stprintf_a_ok(void **);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_stprintf_a_trunc),
        cmocka_unit_test(test_stprintf_a_ok),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_stprintf_a_trunc(void **)
{
	char  buf[countof("foo")];

	// Test that we're not returning SIZE_MAX
	assert_true(stprintf_a(buf, "f%su", "oo") < 0);
	assert_true(strcmp(buf, "foo") == 0);

	assert_true(stprintf_a(buf, "barbaz") == -1);
	assert_true(strcmp(buf, "bar") == 0);
}


static void
test_stprintf_a_ok(void **)
{
	char  buf[countof("foo")];

	assert_true(stprintf_a(buf, "%s", "foo") == strlen("foo"));
	assert_true(strcmp(buf, "foo") == 0);

	assert_true(stprintf_a(buf, "%do", 1) == strlen("1o"));
	assert_true(strcmp(buf, "1o") == 0);

	assert_true(stprintf_a(buf, "f") == strlen("f"));
	assert_true(strcmp(buf, "f") == 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
	assert_true(stprintf_a(buf, "") == strlen(""));
#pragma GCC diagnostic pop
	assert_true(strcmp(buf, "") == 0);
}
