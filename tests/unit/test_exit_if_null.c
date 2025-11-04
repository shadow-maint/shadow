// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "alloc/malloc.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "sizeof.h"
#include "string/strcmp/streq.h"


#define assert_unreachable()  assert_true(0)

#define EXIT_CALLED       (42)


static jmp_buf  jmpb;


void __wrap_exit(int status);

static void test_exit_if_null_exit(void **state);
static void test_exit_if_null_ok(void **state);


int
main(void)
{
	const struct CMUnitTest  tests[] = {
		cmocka_unit_test(test_exit_if_null_exit),
		cmocka_unit_test(test_exit_if_null_ok),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


void
__wrap_exit(int status)
{
	longjmp(jmpb, EXIT_CALLED);
}


static void
test_exit_if_null_exit(void **state)
{
	char *volatile  p;

	p = "before";

	switch (setjmp(jmpb)) {
	case 0:
		p = "called";
		p = XMALLOC(SIZE_MAX, char);
		assert_unreachable();
		break;
	case EXIT_CALLED:
		assert_true(streq(p, "called"));
		p = "test_ok";
		break;
	default:
		assert_unreachable();
		break;
	}

	assert_true(streq(p, "test_ok"));
}


static void
test_exit_if_null_ok(void **state)
{
	char  *p;

	static const char  foo[] = "foo1bar";

	p = XMALLOC(countof(foo), char);
	assert_true(p != NULL);
	strcpy(p, foo);
	assert_string_equal(p, "foo1bar");
	free(p);

	p = XMALLOC(0, char);
	assert_true(p != NULL);
	free(p);
}
