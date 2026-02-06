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


static void
test_is_valid_hash_ok_bcrypt(void **)
{
	// Basic bcrypt hash: $2a$ + cost + $ + 53 character hash
	assert_true(is_valid_hash("$2a$12$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));

	// Bcrypt $2b$ variant with different cost
	assert_true(is_valid_hash("$2b$10$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));

	// Bcrypt $2x$ variant with minimum cost
	assert_true(is_valid_hash("$2x$04$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));

	// Bcrypt $2y$ variant with high cost
	assert_true(is_valid_hash("$2y$15$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));

	// Bcrypt with numeric characters in hash portion
	assert_true(is_valid_hash("$2a$08$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));
}


static void
test_is_valid_hash_ok_sha512(void **)
{
	// Basic SHA-512 hash: $6$ + salt + $ + 86 character hash
	assert_true(is_valid_hash("$6$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with rounds parameter
	assert_true(is_valid_hash("$6$rounds=5000$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with minimum rounds (1000)
	assert_true(is_valid_hash("$6$rounds=1000$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with maximum rounds (999999999)
	assert_true(is_valid_hash("$6$rounds=999999999$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with minimum salt length (1 character)
	assert_true(is_valid_hash("$6$a$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with maximum salt length (16 characters)
	assert_true(is_valid_hash("$6$sixteencharsaltx$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));
}


static void
test_is_valid_hash_ok_sha256(void **)
{
	// Basic SHA-256 hash: $5$ + salt + $ + 43 character hash
	assert_true(is_valid_hash("$5$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with rounds parameter
	assert_true(is_valid_hash("$5$rounds=5000$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with minimum rounds (1000)
	assert_true(is_valid_hash("$5$rounds=1000$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with maximum rounds (999999999)
	assert_true(is_valid_hash("$5$rounds=999999999$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with minimum salt length (1 character)
	assert_true(is_valid_hash("$5$a$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with maximum salt length (16 characters)
	assert_true(is_valid_hash("$5$sixteencharsaltx$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));
}


static void
test_is_valid_hash_ok_md5(void **)
{
	// Basic MD5 hash: $1$ + salt + $ + 22 character hash
	assert_true(is_valid_hash("$1$salt$abcdefghijklmnopqrstuv"));

	// MD5 with maximum salt length (8 characters)
	assert_true(is_valid_hash("$1$maxsalt8$abcdefghijklmnopqrstuv"));
}


static void
test_is_valid_hash_ok_des(void **)
{
	// Basic DES hash: 13 characters
	assert_true(is_valid_hash("abcDEF123./zZ"));

	// DES with different character combinations
	assert_true(is_valid_hash("ZZ./0123456xy"));
}


static void
test_is_valid_hash_ok_special(void **)
{
	// Empty string - passwordless account
	assert_true(is_valid_hash(""));

	// Single asterisk - password permanently locked
	assert_true(is_valid_hash("*"));

	// Single ! - temporarily locked passwordless account
	assert_true(is_valid_hash("!"));

	// Leading ! prefix - temporarily locked account with SHA-512 hash
	assert_true(is_valid_hash("!$6$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// Leading ! prefix - temporarily locked account with yescrypt hash
	assert_true(is_valid_hash("!$y$j9T$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));
}


static void
test_is_valid_hash_edge_salt_chars(void **)
{
	// SHA-512 with backslash in salt
	assert_true(is_valid_hash("$6$sa\\lt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-512 with 'n' in salt
	assert_true(is_valid_hash("$6$salnt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890./abcdefghijklmnopqrstuv"));

	// SHA-256 with backslash in salt
	assert_true(is_valid_hash("$5$sa\\lt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));

	// SHA-256 with 'n' in salt
	assert_true(is_valid_hash("$5$salnt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));
}


static void
test_is_valid_hash_edge_account_locks(void **)
{
	// Complex ! prefix scenarios with various hash types should work
	assert_true(is_valid_hash("!$2a$12$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ."));
	assert_true(is_valid_hash("!$5$salt$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQ"));
	assert_true(is_valid_hash("!$1$salt$abcdefghijklmnopqrstuv"));

	// But invalid hashes with ! prefix should still fail
	assert_false(is_valid_hash("!invalid"));
	assert_false(is_valid_hash("!toolong"));
}


int
main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_is_valid_hash_ok_yescrypt),
        cmocka_unit_test(test_is_valid_hash_ok_bcrypt),
        cmocka_unit_test(test_is_valid_hash_ok_sha512),
        cmocka_unit_test(test_is_valid_hash_ok_sha256),
        cmocka_unit_test(test_is_valid_hash_ok_md5),
        cmocka_unit_test(test_is_valid_hash_ok_des),
        cmocka_unit_test(test_is_valid_hash_ok_special),
        cmocka_unit_test(test_is_valid_hash_edge_salt_chars),
        cmocka_unit_test(test_is_valid_hash_edge_account_locks),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
