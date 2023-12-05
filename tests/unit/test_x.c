/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "attr.h"
#include "x.h"


#define assert_unreachable()  assert_true(0)

#define X_CALLED     (-36)
#define EXIT_CALLED  (42)
#define TEST_OK      (-6)


volatile int    n;
volatile void   *p;
static jmp_buf  jmpb;


void __wrap_exit(int status);

static void test_x_1(unused void **state);
static void test_x_null(unused void **state);
static void test_x_ok(unused void **state);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_x_1),
        cmocka_unit_test(test_x_null),
        cmocka_unit_test(test_x_ok),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


void
__wrap_exit(int status)
{
	longjmp(jmpb, EXIT_CALLED);
}


static void
test_x_1(void **state)
{
	n = 0;

	switch (setjmp(jmpb)) {
	case 0:
		n = X_CALLED;
		n = x(-1);
		assert_unreachable();
		break;
	case EXIT_CALLED:
		assert_true(n == X_CALLED);
		n = TEST_OK;
		break;
	default:
		assert_unreachable();
		break;
	}

	assert_true(n == TEST_OK);
}


static void
test_x_null(void **state)
{
	p = &getppid;

	switch (setjmp(jmpb)) {
	case 0:
		p = &getpid;
		p = x(NULL);
		assert_unreachable();
		break;
	case EXIT_CALLED:
		assert_true(p == &getpid);
		p = &time;
		break;
	default:
		assert_unreachable();
		break;
	}

	assert_true(p == &time);
}


static void
test_x_ok(void **state)
{
	assert_true(x(0) == 0);
	assert_true(x(getpid()) == getpid());
	assert_true(x(42) == 42);
	assert_true(x(-6) == -6);
	assert_true(x(&test_x_ok) == &test_x_ok);
}
