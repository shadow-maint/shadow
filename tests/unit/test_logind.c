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

#include "prototypes.h"

/***********************
 * WRAPPERS
 **********************/
struct passwd *
__wrap_prefix_getpwnam(uid_t uid)
{
    return (struct passwd*) mock();
}

int
__wrap_sd_uid_get_sessions(uid_t uid, int require_active, char ***sessions)
{
    return mock();
}

/***********************
 * TEST
 **********************/
static void test_active_sessions_count_return_ok(void **state)
{
    int res;
    struct passwd *pw = malloc(sizeof(struct passwd));

    will_return(__wrap_prefix_getpwnam, pw);
    will_return(__wrap_sd_uid_get_sessions, 1);

    res = active_sessions_limit_exceeded_lgnd("testuser", 1);

    assert_int_equal(res, 0);
}

static void
test_active_sessions_limit_exceeded_lgnd_return_exceeded(void **state)
{
    int res;
    struct passwd *pw = malloc(sizeof(struct passwd));

    will_return(__wrap_prefix_getpwnam, pw);
    will_return(__wrap_sd_uid_get_sessions, 1);

    res = active_sessions_limit_exceeded_lgnd("testuser", 0);

    assert_int_equal(res, 1);
}

static void
test_active_sessions_limit_exceeded_lgnd_prefix_failure(void **state)
{
    int res;
    struct passwd *pw = NULL;

    will_return(__wrap_prefix_getpwnam, pw);

    res = active_sessions_limit_exceeded_lgnd("testuser", 0);

    assert_int_equal(res, -1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_active_sessions_limit_exceeded_lgnd_return_ok),
        cmocka_unit_test(\
        test_active_sessions_limit_exceeded_lgnd_return_exceeded),
        cmocka_unit_test(\
        test_active_sessions_limit_exceeded_lgnd_prefix_failure),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
