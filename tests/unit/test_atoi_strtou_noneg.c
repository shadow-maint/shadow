// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
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


static void test_strtoul(void **state);
static void test_strtoul_noneg(void **state);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_strtoul),
        cmocka_unit_test(test_strtoul_noneg),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_strtoul(void **state)
{
	errno = 0;
	assert_true(strtoul("42", NULL, 0) == 42);
	assert_true(errno == 0);

	assert_true(strtoul("-1", NULL, 0) == -1ul);
	assert_true(errno == 0);
	assert_true(strtoul("-3", NULL, 0) == -3ul);
	assert_true(errno == 0);
	switch (sizeof(long)) {
	case 8:
		assert_true(strtoul("-0xFFFFFFFFFFFFFFFF", NULL, 0) == 1);
		break;
	case 4:
		assert_true(strtoul("-0xFFFFFFFF", NULL, 0) == 1);
		break;
	}
	assert_true(errno == 0);

	assert_true(strtoul("-0x10000000000000000", NULL, 0) == ULONG_MAX);
	assert_true(errno == ERANGE);
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
