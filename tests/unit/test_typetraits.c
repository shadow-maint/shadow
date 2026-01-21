// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "typetraits.h"


static void test_maxof(void **);
static void test_minof(void **);


int
main(void)
{
	const struct CMUnitTest  tests[] = {
		cmocka_unit_test(test_maxof),
		cmocka_unit_test(test_minof),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_maxof(void **)
{
	assert_true(maxof(long) == LONG_MAX);
	assert_true(maxof(unsigned long) == ULONG_MAX);
	assert_true(maxof(int) == INT_MAX);
	assert_true(maxof(unsigned short) == USHRT_MAX);
	assert_true(maxof(char) == CHAR_MAX);
	assert_true(maxof(signed char) == SCHAR_MAX);
	assert_true(maxof(unsigned char) == UCHAR_MAX);
}


static void
test_minof(void **)
{
	assert_true(minof(long) == LONG_MIN);
	assert_true(minof(unsigned long) == 0);
	assert_true(minof(int) == INT_MIN);
	assert_true(minof(unsigned short) == 0);
	assert_true(minof(char) == CHAR_MIN);
	assert_true(minof(signed char) == SCHAR_MIN);
	assert_true(minof(unsigned char) == 0);
}
