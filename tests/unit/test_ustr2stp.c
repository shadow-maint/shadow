/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "sizeof.h"
#include "string/ustr2stp.h"


static void test_ustr2stp(void **state);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_ustr2stp),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_ustr2stp(void **state)
{
	char  src[3] = {'f', 'o', 'o'};  // No NUL terminator.
	char  dst[NITEMS(src) + 1];

	assert_true(ustr2stp(dst, src, 3) == dst + 3);
	assert_true(strcmp(dst, "foo") == 0);
}
