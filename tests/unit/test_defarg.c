// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "attr.h"
#include "macro.h"


static void test_value(MAYBE_UNUSED void ** _1);
static void test_evaluation(MAYBE_UNUSED void ** _1);


int
main(void)
{
	const struct CMUnitTest  tests[] = {
		cmocka_unit_test(test_value),
		cmocka_unit_test(test_evaluation),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_value(MAYBE_UNUSED void ** _1)
{
	assert_int_equal(DEFARG(6, 7), 7);
	assert_int_equal(DEFARG(6,  ), 6);
}


static void
test_evaluation(MAYBE_UNUSED void ** _1)
{
	int  i;

	i = 6;
	assert_int_equal(DEFARG(i++, 42), 42);
	assert_int_equal(i, 6);

	i = 6;
	assert_int_equal(DEFARG(i++,   ), 6);
	assert_int_equal(i, 7);

	i = 6;
	assert_int_equal(DEFARG(42, i++), 6);
	assert_int_equal(i, 7);
}
