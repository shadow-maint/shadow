/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "alloc/malloc.h"
#include "chkname.h"


static void test_is_valid_user_name_ok(void **);
static void test_is_valid_user_name_ok_dollar(void **);
static void test_is_valid_user_name_nok_dash(void **);
static void test_is_valid_user_name_nok_dir(void **);
static void test_is_valid_user_name_nok_dollar(void **);
static void test_is_valid_user_name_nok_empty(void **);
static void test_is_valid_user_name_nok_numeric(void **);
static void test_is_valid_user_name_nok_otherchars(void **);
static void test_is_valid_user_name_long(void **);


int
main(void)
{
    const struct CMUnitTest  tests[] = {
        cmocka_unit_test(test_is_valid_user_name_ok),
        cmocka_unit_test(test_is_valid_user_name_ok_dollar),
        cmocka_unit_test(test_is_valid_user_name_nok_dash),
        cmocka_unit_test(test_is_valid_user_name_nok_dir),
        cmocka_unit_test(test_is_valid_user_name_nok_dollar),
        cmocka_unit_test(test_is_valid_user_name_nok_empty),
        cmocka_unit_test(test_is_valid_user_name_nok_numeric),
        cmocka_unit_test(test_is_valid_user_name_nok_otherchars),
        cmocka_unit_test(test_is_valid_user_name_long),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_is_valid_user_name_ok(void **)
{
	assert_true(is_valid_user_name("alx"));
	assert_true(is_valid_user_name("u-ser"));
	assert_true(is_valid_user_name("u"));
	assert_true(is_valid_user_name("I"));
	assert_true(is_valid_user_name("_"));
	assert_true(is_valid_user_name("_.-"));
	assert_true(is_valid_user_name(".007"));
	assert_true(is_valid_user_name("0_0"));
	assert_true(is_valid_user_name("some_longish_user_name_sHould_also_be_valid.wHY_not"));
}


static void
test_is_valid_user_name_ok_dollar(void **)
{
	// Non-POSIX extension for Samba 3.x "add machine script".
	assert_true(is_valid_user_name("dollar$"));
	assert_true(is_valid_user_name("SSS$"));
}


static void
test_is_valid_user_name_nok_dash(void **)
{
	assert_true(false == is_valid_user_name("-"));
	assert_true(false == is_valid_user_name("-not-valid"));
	assert_true(false == is_valid_user_name("--C"));
}


static void
test_is_valid_user_name_nok_dir(void **)
{
	assert_true(false == is_valid_user_name("."));
	assert_true(false == is_valid_user_name(".."));
}


static void
test_is_valid_user_name_nok_dollar(void **)
{
	assert_true(false == is_valid_user_name("$"));
	assert_true(false == is_valid_user_name("$dollar"));
	assert_true(false == is_valid_user_name("mo$ney"));
	assert_true(false == is_valid_user_name("do$$ar"));
	assert_true(false == is_valid_user_name("foo$bar$"));
}


static void
test_is_valid_user_name_nok_empty(void **)
{
	assert_true(false == is_valid_user_name(""));
}


static void
test_is_valid_user_name_nok_numeric(void **)
{
	assert_true(false == is_valid_user_name("6"));
	assert_true(false == is_valid_user_name("42"));
}


static void
test_is_valid_user_name_nok_otherchars(void **)
{
	assert_true(false == is_valid_user_name("no spaces"));
	assert_true(false == is_valid_user_name("no,"));
	assert_true(false == is_valid_user_name("no;"));
	assert_true(false == is_valid_user_name("no:"));
}


static void
test_is_valid_user_name_long(void **)
{
	size_t  max;
	char    *name;

	max = sysconf(_SC_LOGIN_NAME_MAX);
	name = MALLOC(max + 1, char);
	assert_true(name != NULL);

	memset(name, '_', max);

	stpcpy(&name[max], "");
	assert_true(false == is_valid_user_name(name));

	stpcpy(&name[max - 1], "");
	assert_true(is_valid_user_name(name));

	free(name);
}
