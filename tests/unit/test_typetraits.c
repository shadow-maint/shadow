// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "typetraits.h"


static void test_type_max(void **state);
static void test_type_min(void **state);


int
main(void)
{
	const struct CMUnitTest  tests[] = {
		cmocka_unit_test(test_type_max),
		cmocka_unit_test(test_type_min),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_type_max(void **state)
{
	assert_true(type_max(long) == LONG_MAX);
	assert_true(type_max(unsigned long) == ULONG_MAX);
	assert_true(type_max(int) == INT_MAX);
	assert_true(type_max(unsigned short) == USHRT_MAX);
	assert_true(type_max(char) == CHAR_MAX);
	assert_true(type_max(signed char) == SCHAR_MAX);
	assert_true(type_max(unsigned char) == UCHAR_MAX);
}


static void
test_type_min(void **state)
{
	assert_true(type_min(long) == LONG_MIN);
	assert_true(type_min(unsigned long) == 0);
	assert_true(type_min(int) == INT_MIN);
	assert_true(type_min(unsigned short) == 0);
	assert_true(type_min(char) == CHAR_MIN);
	assert_true(type_min(signed char) == SCHAR_MIN);
	assert_true(type_min(unsigned char) == 0);
}
