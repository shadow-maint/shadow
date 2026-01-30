// SPDX-FileCopyrightText: 2026, Iker Pedrosa <ipedrosa@redhat.com>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>  // Required by <cmocka.h>
#include <stddef.h>  // Required by <cmocka.h>
#include <setjmp.h>  // Required by <cmocka.h>
#include <stdint.h>  // Required by <cmocka.h>
#include <cmocka.h>

#include "chkhash.h"


static void
test_is_valid_hash_ok_yescrypt(void **)
{
	// Basic yescrypt hash: $y$ + params + $ + salt + $ + 43 character hash
	assert_true(is_valid_hash("$y$j9T$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with longer salt
	assert_true(is_valid_hash("$y$j9T$longsalt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with maximum salt length (86 characters)
	assert_true(is_valid_hash("$y$j9T$abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890123456789012$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with minimum salt length (1 character)
	assert_true(is_valid_hash("$y$j9T$a$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with different parameter configuration
	assert_true(is_valid_hash("$y$v1.jCf$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with numeric parameters and mixed case salt
	assert_true(is_valid_hash("$y$123$SaLt123$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// Yescrypt with special characters in salt (./A-Za-z0-9 charset)
	assert_true(is_valid_hash("$y$j9T$salt./ABC123$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));
}


int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_is_valid_hash_ok_yescrypt),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
