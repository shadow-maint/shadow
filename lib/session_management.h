// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


/*
 * Session management: utmp.c or logind.c
 */


#ifndef SHADOW_INCLUDE_LIB_SESSION_MANAGEMENT_H_
#define SHADOW_INCLUDE_LIB_SESSION_MANAGEMENT_H_


#include "config.h"

#include <sys/types.h>


/**
 * @brief Get host for the current session
 *
 * @param[out] out Host name
 * @param[in] main_pid the PID of the main process (the parent PID if
 *                     the process forked itself)
 *
 * @return 0 or a positive integer if the host was obtained properly,
 *         another value on error.
 */
int get_session_host(char **out, pid_t main_pid);


#if !defined(ENABLE_LOGIND)
/**
 * @brief Update or create an utmp entry in utmp, wtmp, utmpw, or wtmpx
 *
 * @param[in] user username
 * @param[in] tty tty
 * @param[in] host hostname
 * @param[in] main_pid the PID of the main process (the parent PID if
 *                     the process forked itself)
 *
 * @return 0 if utmp was updated properly,
 *         1 on error.
 */
int update_utmp(const char *user, const char *tty, const char *host,
    pid_t main_pid);
/**
 * @brief Update the cumulative failure log
 *
 * @param[in] failent_user username
 * @param[in] tty tty
 * @param[in] host hostname
 * @param[in] main_pid the PID of the main process (the parent PID if
 *                     the process forked itself)
 *
 */
void record_failure(const char *failent_user, const char *tty,
    const char *hostname, pid_t main_pid);
#endif /* !ENABLE_LOGIND */


/**
 * @brief Number of active user sessions
 *
 * @param[in] name username
 * @param[in] limit maximum number of active sessions
 *
 * @return number of active sessions.
 *
 */
unsigned long active_sessions_count(const char *name, unsigned long limit);


#endif  // include guard
