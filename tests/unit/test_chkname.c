/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <limits.h>
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
#include "attr.h"

static void test_is_valid_user_name_ok(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_ok_dollar(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_dash(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_dir(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_dollar(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_empty(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_numeric(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_nok_otherchars(MAYBE_UNUSED void ** _1);
static void test_is_valid_user_name_long(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_ok(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_nok_not_upn(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_nok_structure(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_nok_domain(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_ok_limits(MAYBE_UNUSED void ** _1);
static void test_is_valid_upn_nok_limits(MAYBE_UNUSED void ** _1);


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
        cmocka_unit_test(test_is_valid_upn_ok),
        cmocka_unit_test(test_is_valid_upn_nok_not_upn),
        cmocka_unit_test(test_is_valid_upn_nok_structure),
        cmocka_unit_test(test_is_valid_upn_nok_domain),
        cmocka_unit_test(test_is_valid_upn_ok_limits),
        cmocka_unit_test(test_is_valid_upn_nok_limits),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


static void
test_is_valid_user_name_ok(MAYBE_UNUSED void ** _1)
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
test_is_valid_user_name_ok_dollar(MAYBE_UNUSED void ** _1)
{
	// Non-POSIX extension for Samba 3.x "add machine script".
	assert_true(is_valid_user_name("dollar$"));
	assert_true(is_valid_user_name("SSS$"));
}


static void
test_is_valid_user_name_nok_dash(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name("-"));
	assert_true(false == is_valid_user_name("-not-valid"));
	assert_true(false == is_valid_user_name("--C"));
}


static void
test_is_valid_user_name_nok_dir(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name("."));
	assert_true(false == is_valid_user_name(".."));
}


static void
test_is_valid_user_name_nok_dollar(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name("$"));
	assert_true(false == is_valid_user_name("$dollar"));
	assert_true(false == is_valid_user_name("mo$ney"));
	assert_true(false == is_valid_user_name("do$$ar"));
	assert_true(false == is_valid_user_name("foo$bar$"));
}


static void
test_is_valid_user_name_nok_empty(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name(""));
}


static void
test_is_valid_user_name_nok_numeric(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name("6"));
	assert_true(false == is_valid_user_name("42"));
}


static void
test_is_valid_user_name_nok_otherchars(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_user_name("no spaces"));
	assert_true(false == is_valid_user_name("no,"));
	assert_true(false == is_valid_user_name("no;"));
	assert_true(false == is_valid_user_name("no:"));
}


static void
test_is_valid_user_name_long(MAYBE_UNUSED void ** _1)
{
	char  *name;

	name = malloc_T(LOGIN_NAME_MAX + 1, char);
	assert_true(name != NULL);

	memset(name, '_', LOGIN_NAME_MAX);

	stpcpy(&name[LOGIN_NAME_MAX], "");
	assert_true(false == is_valid_user_name(name));

	stpcpy(&name[LOGIN_NAME_MAX - 1], "");
	assert_true(is_valid_user_name(name));

	free(name);
}


static void
test_is_valid_upn_ok(MAYBE_UNUSED void ** _1)
{
	assert_true(is_valid_upn("user@example.com"));
	assert_true(is_valid_upn("john.doe@corp.example.org"));
	assert_true(is_valid_upn("test@sub.domain.net"));
	assert_true(is_valid_upn("a@b.c"));
	assert_true(is_valid_upn("user123@test123.example"));
	assert_true(is_valid_upn("user_name@example-domain.com"));
	assert_true(is_valid_upn("test.user@example.domain.org"));
}


static void
test_is_valid_upn_nok_not_upn(MAYBE_UNUSED void ** _1)
{
	assert_true(false == is_valid_upn("regularuser"));
	assert_true(false == is_valid_upn("user.name"));
	assert_true(false == is_valid_upn("user_name"));
	assert_true(false == is_valid_upn("123user"));
	assert_true(false == is_valid_upn("USER"));
}


static void
test_is_valid_upn_nok_structure(MAYBE_UNUSED void ** _1)
{
	// Empty parts
	assert_true(false == is_valid_upn("@domain.com"));
	assert_true(false == is_valid_upn("user@"));
	assert_true(false == is_valid_upn("@"));

	// Multiple @ symbols
	assert_true(false == is_valid_upn("user@domain@com"));
	assert_true(false == is_valid_upn("@@domain.com"));
	assert_true(false == is_valid_upn("user@@domain.com"));

	// Empty string
	assert_true(false == is_valid_upn(""));

	// Invalid suffix
	assert_true(false == is_valid_upn("-user@domain.com"));
	assert_true(false == is_valid_upn("123@domain.com"));
	assert_true(false == is_valid_upn("user space@domain.com"));
}


static void
test_is_valid_upn_nok_domain(MAYBE_UNUSED void ** _1)
{
	// Invalid domain formats
	assert_true(false == is_valid_upn("user@domain"));
	assert_true(false == is_valid_upn("user@.domain.com"));
	assert_true(false == is_valid_upn("user@domain.com."));
	assert_true(false == is_valid_upn("user@domain..com"));
	assert_true(false == is_valid_upn("user@sub."));

	// Invalid domain characters
	assert_true(false == is_valid_upn("user@domain_name.com"));
	assert_true(false == is_valid_upn("user@domain name.com"));
}


static void
test_is_valid_upn_ok_limits(MAYBE_UNUSED void ** _1)
{
	char    *upn;
	char    *domain;

	// Test domain at maximum allowed length (253 chars)
	domain = malloc_T(253 + 1, char);
	assert_true(domain != NULL);
	strcpy(domain,
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa." // 63 a's + dot
		"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb." // 63 b's + dot
		"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc." // 63 c's + dot
		"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddd."      // 58 d's + dot
		"co");                                                             // 2
	// Total: 63 + 1 + 63 + 1 + 63 + 1 + 58 + 1 + 2 = 253 chars

	upn = malloc_T(5 + 253 + 1, char);  // "user@" + domain + '\0'
	assert_true(upn != NULL);
	strcpy(upn, "user@");
	strcat(upn, domain);
	assert_true(is_valid_upn(upn));

	free(upn);
	free(domain);
}


static void
test_is_valid_upn_nok_limits(MAYBE_UNUSED void ** _1)
{
	char    *upn;
	char    *domain;

	// Test domain exceeding maximum length (254 chars)
	domain = malloc_T(254 + 1, char);
	assert_true(domain != NULL);
	memset(domain, 'a', 251);
	strcpy(&domain[251], ".co");

	upn = malloc_T(5 + 254 + 1, char);  // "user@" + domain + '\0'
	assert_true(upn != NULL);
	strcpy(upn, "user@");
	strcat(upn, domain);
	assert_true(false == is_valid_upn(upn));

	free(upn);
	free(domain);

	// Domain label too long (>63 chars)
	assert_true(false == is_valid_upn("user@"
		"verylongdomainlabelthatexceedssixtythreecharacterslimitsetbyRFC1035"
		".com"));
}
