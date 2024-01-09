// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "atoi/strtou_noneg.h"


static void test_strtoul_noneg(void **state);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_strtoul_noneg),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_strtoul_noneg(void **state)
{
	errno = 0;
	assert_true(strtoul_noneg("42", NULL, 0) == 42);
	assert_true(errno == 0);

	assert_true(strtoul_noneg("-1", NULL, 0) == 0);
	assert_true(errno == ERANGE);
	errno = 0;
	assert_true(strtoul_noneg("-3", NULL, 0) == 0);
	assert_true(errno == ERANGE);
	errno = 0;
	assert_true(strtoul_noneg("-0xFFFFFFFFFFFFFFFF", NULL, 0) == 0);
	assert_true(errno == ERANGE);

	errno = 0;
	assert_true(strtoul_noneg("-0x10000000000000000", NULL, 0) == 0);
	assert_true(errno == ERANGE);
}
