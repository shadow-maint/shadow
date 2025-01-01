// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "string/sprintf/xaprintf.h"


#define smock()               _Generic(mock(), uintmax_t: (intmax_t) mock())
#define assert_unreachable()  assert_true(0)

#define EXIT_CALLED       (42)


static jmp_buf  jmpb;


int __real_vasprintf(char **restrict p, const char *restrict fmt, va_list ap);
int __wrap_vasprintf(char **restrict p, const char *restrict fmt, va_list ap);
void __wrap_exit(int status);

static void test_xaprintf_exit(void **state);
static void test_xaprintf_ok(void **state);


int
main(void)
{
	const struct CMUnitTest  tests[] = {
		cmocka_unit_test(test_xaprintf_exit),
		cmocka_unit_test(test_xaprintf_ok),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}


int
__wrap_vasprintf(char **restrict p, const char *restrict fmt, va_list ap)
{
	return smock() == -1 ? -1 : __real_vasprintf(p, fmt, ap);
}


void
__wrap_exit(int status)
{
	longjmp(jmpb, EXIT_CALLED);
}


static void
test_xaprintf_exit(void **state)
{
	char *volatile  p;

	will_return(__wrap_vasprintf, -1);

	p = "before";

	switch (setjmp(jmpb)) {
	case 0:
		p = "xaprintf_called";
		p = xaprintf("foo%s", "bar");
		assert_unreachable();
		break;
	case EXIT_CALLED:
		assert_true(strcmp(p, "xaprintf_called"));
		p = "test_ok";
		break;
	default:
		assert_unreachable();
		break;
	}

	assert_true(strcmp(p, "test_ok"));
}


static void
test_xaprintf_ok(void **state)
{
	char  *p;

	// Trick: vasprintf(3) will actually return the new string, not 0.
	will_return(__wrap_vasprintf, 0);

	p = xaprintf("foo%d%s", 1, "bar");
	assert_string_equal(p, "foo1bar");
	free(p);
}
