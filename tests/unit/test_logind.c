/*
 * SPDX-FileCopyrightText:  2023, Iker Pedrosa <ipedrosa@redhat.com>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "session_management.h"

/***********************
 * WRAPPERS
 **********************/
struct passwd *
__wrap_prefix_getpwnam(uid_t)
{
    return (struct passwd*) mock();
}

int
__wrap_sd_uid_get_sessions(uid_t, int, char ***)
{
    return mock();
}

/***********************
 * TEST
 **********************/
static void test_active_sessions_count_return_ok(void **)
{
    int count;
    struct passwd *pw = malloc(sizeof(struct passwd));

    will_return(__wrap_prefix_getpwnam, pw);
    will_return(__wrap_sd_uid_get_sessions, 1);

    count = active_sessions_count("testuser", 0);

    assert_int_equal(count, 1);
}

static void test_active_sessions_count_prefix_getpwnam_failure(void **)
{
    int count;
    struct passwd *pw = NULL;

    will_return(__wrap_prefix_getpwnam, pw);

    count = active_sessions_count("testuser", 0);

    assert_int_equal(count, 0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_active_sessions_count_return_ok),
        cmocka_unit_test(test_active_sessions_count_prefix_getpwnam_failure),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
