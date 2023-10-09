/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "sprintf.h"


/**********************
 * TEST
 **********************/
static void
test_xasprintf(void **state)
{
	int   len;
	char  *p;

	len = xasprintf(&p, "foo%d%s", 1, "bar");
	assert_int_equal(len, strlen("foo1bar"));
	assert_string_equal(p, "foo1bar");
	free(p);
}


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_xasprintf),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
