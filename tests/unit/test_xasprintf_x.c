/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "sprintf.h"


#define assert_unreachable()  assert_true(0)


static jmp_buf  jmpb;


/**********************
 * WRAPPERS
 **********************/
int
__wrap_vasprintf(char **restrict p, const char *restrict fmt, va_list ap)
{
	return -1;
}


void
__wrap_exit(int status)
{
	longjmp(jmpb, 42);
}


/**********************
 * TEST
 **********************/
static void
test_xasprintf_x(void **state)
{
	volatile int    len;
	char *volatile  p;

	len = 0;

	switch (setjmp(jmpb)) {
	case 0:
		len = -36;
		len = xasprintf(&p, "foo%s", "bar");
		assert_unreachable();
		break;
	case 42:
		assert_int_equal(len, -36);
		len = -6;
		break;
	default:
		assert_unreachable();
		break;
	}

	assert_int_equal(len, -6);
}


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_xasprintf_x),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
