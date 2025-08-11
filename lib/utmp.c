/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 * SPDX-FileCopyrightText: 2025, Evgeny Grin (Karlson2k)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "defines.h"
#include "prototypes.h"
#include "getdef.h"

#include <utmpx.h>
#include <assert.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "alloc/calloc.h"
#include "alloc/malloc.h"
#include "attr.h"
#include "sizeof.h"
#include "string/strchr/strnul.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strneq.h"
#include "string/strcmp/strprefix.h"
#include "string/strcpy/strncpy.h"
#include "string/strdup/memdup.h"
#include "string/strdup/strdup.h"
#include "string/strdup/strndup.h"

#ident "$Id$"


#define UTX_LINESIZE  countof(memberof(struct utmpx, ut_line))

// ttyname_ra - tty name re-entrant array
#define ttyname_ra(fd, buf)  ttyname_r(fd, buf, countof(buf))


/*
 * is_my_tty -- determine if "tty" is the same TTY stdin is using
 */
static bool
is_my_tty(const char tty[UTX_LINESIZE])
{
	char  full_tty[STRLEN("/dev/") + UTX_LINESIZE + 1];
	char  my_tty[countof(full_tty)];

	stpcpy(full_tty, "");
	if (tty[0] != '/')
		strcpy (full_tty, "/dev/");
	strncat(full_tty, tty, UTX_LINESIZE);

	if (ttyname_ra(STDIN_FILENO, my_tty) != 0) {
		(void) puts (_("Unable to determine your tty name."));
		exit (EXIT_FAILURE);
	}

	return streq(full_tty, my_tty);
}


/*
 * failtmp - update the cumulative failure log
 *
 *	failtmp updates the (struct utmpx) formatted failure log which
 *	maintains a record of all login failures.
 */
static void
failtmp(const char *username, const struct utmpx *failent)
{
	const char *ftmp;
	int fd;

	/*
	 * Get the name of the failure file.  If no file has been defined
	 * in login.defs, don't do this.
	 */

	ftmp = getdef_str ("FTMP_FILE");
	if (NULL == ftmp) {
		return;
	}

	/*
	 * Open the file for append.  It must already exist for this
	 * feature to be used.
	 */

	fd = open (ftmp, O_WRONLY | O_APPEND);
	if (-1 == fd) {
		if (errno != ENOENT) {
			SYSLOG ((LOG_WARN,
			        "Can't append failure of user %s to %s: %m",
			        username, ftmp));
		}
		return;
	}

	/*
	 * Append the new failure record and close the log file.
	 */

	if (write_full(fd, failent, sizeof(*failent)) == -1) {
		goto err_write;
	}

	if (close (fd) != 0 && errno != EINTR) {
		goto err_close;
	}

	return;

err_write:
	{
		int saved_errno = errno;
		(void) close (fd);
		errno = saved_errno;
	}
err_close:
	SYSLOG ((LOG_WARN,
	         "Can't append failure of user %s to %s: %m",
	         username, ftmp));
}


/*
 * get_current_utmp - return the most probable utmp entry for the current
 *                    session
 *
 *	When an entry is returned by this function, and if the utmpx structure
 *	has a ut_id field and this field is not empty, then this field
 *	should be used to update the entry information.
 *
 *	Return NULL if no entries exist in utmp for the current process or
 *	            there is an error reading utmp.
 */
ATTR_MALLOC(free)
static /*@null@*/ /*@only@*/struct utmpx *
get_current_utmp(pid_t main_pid)
{
	struct utmpx  *ut;
	struct utmpx  *ut_by_pid  = NULL;
	struct utmpx  *ut_by_line = NULL;

	setutxent();

	/* First, try to find a valid utmp entry for this process.  */
	while (NULL != (ut = getutxent())) {
		if (   (LOGIN_PROCESS != ut->ut_type)
		    && (USER_PROCESS  != ut->ut_type))
			continue;

		if (main_pid == ut->ut_pid) {
			if (is_my_tty(ut->ut_line))
				break; /* Perfect match, stop the search */

			if (NULL == ut_by_pid) {
				ut_by_pid = xmalloc_T(1, struct utmpx);
				*ut_by_pid = *ut;
			}

		} else if (   LOGIN_PROCESS == ut->ut_type /* Be more picky when matching by 'ut_line' only */
			   && is_my_tty(ut->ut_line))
		{
			if (NULL == ut_by_line) {
				ut_by_line = xmalloc_T(1, struct utmpx);
				*ut_by_line = *ut;
			}
		}
	}

	if (NULL == ut)
		ut = ut_by_pid ?: ut_by_line;

	if (NULL != ut)
		ut = memdup_T(ut, struct utmpx);

	free(ut_by_line);
	free(ut_by_pid);
	endutxent();

	return ut;
}


int
get_session_host(char **out, pid_t main_pid)
{
	int           ret = 0;
	struct utmpx  *ut;

	ut = get_current_utmp(main_pid);

#if defined(HAVE_STRUCT_UTMPX_UT_HOST)
	if ((ut != NULL) && !strneq_a(ut->ut_host, "")) {
		*out = xstrndup_a(ut->ut_host);
	} else {
		*out = NULL;
		ret = -2;
	}
#else
	*out = NULL;
	ret = -2;
#endif

	free(ut);

	return ret;
}


#if !defined(USE_PAM) && !defined(HAVE_UPDWTMPX)
/*
 * Some systems already have updwtmpx().  Others
 * don't, so we re-implement these functions if necessary.
 */
static void
updwtmpx(const char *filename, const struct utmpx *ut)
{
	int fd;

	fd = open (filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write_full(fd, ut, sizeof(*ut));
		close (fd);
	}
}
#endif


/*
 * prepare_utmp - prepare an utmp entry so that it can be logged in a
 *                utmp/wtmp file.
 *
 *	It accepts an utmp entry in input (ut) to return an entry with
 *	the right ut_id. This is typically an entry returned by
 *	get_current_utmp
 *	If ut is NULL or ut->ut_id is empty, ut_id will be forged based on
 *	the line argument.
 *
 *	The ut_host field of the input structure may also be kept, and is
 *	used to define the ut_addr/ut_addr_v6 fields. (if these fields
 *	exist)
 *
 *	Other fields are discarded and filed with new values (if they
 *	exist).
 *
 *	The returned structure shall be freed by the caller.
 */
static /*@only@*/struct utmpx *
prepare_utmp(const char *name, const char *line, const char *host,
             /*@null@*/const struct utmpx *ut, pid_t main_pid)
{
	char            *hostname = NULL;
	struct utmpx    *utent;
	struct timeval  tv;

	assert (NULL != name);
	assert (NULL != line);



	if (NULL != host && !streq(host, ""))
		hostname = xstrdup(host);
#if defined(HAVE_STRUCT_UTMPX_UT_HOST)
	else if (NULL != ut && !strneq_a(ut->ut_host, ""))
		hostname = xstrndup_a(ut->ut_host);
#endif

	line = strprefix(line, "/dev/") ?: line;

	utent = xcalloc_T(1, struct utmpx);


	utent->ut_type = USER_PROCESS;
	utent->ut_pid = main_pid;
	strncpy_a(utent->ut_line, line);
	if (   (NULL != ut)
	    && ('\0' != ut->ut_id[0])) {
		strncpy_a(utent->ut_id, ut->ut_id);
	} else {
		strncpy_a(utent->ut_id, strnul(line) - MIN(strlen(line), countof(utent->ut_id)));
	}
#if defined(HAVE_STRUCT_UTMPX_UT_NAME)
	strncpy_a(utent->ut_name, name);
#endif
	strncpy_a(utent->ut_user, name);
	if (NULL != hostname) {
		struct addrinfo *info = NULL;
#if defined(HAVE_STRUCT_UTMPX_UT_HOST)
		strncpy_a(utent->ut_host, hostname);
#endif
#if defined(HAVE_STRUCT_UTMPX_UT_SYSLEN)
		utent->ut_syslen = MIN(strlen(hostname),
		                       sizeof(utent->ut_host));
#endif
#if defined(HAVE_STRUCT_UTMPX_UT_ADDR) || defined(HAVE_STRUCT_UTMPX_UT_ADDR_V6)
		if (getaddrinfo (hostname, NULL, NULL, &info) == 0) {
			/* getaddrinfo might not be reliable.
			 * Just try to log what may be useful.
			 */
			if (info->ai_family == AF_INET) {
				struct sockaddr_in *sa =
					(struct sockaddr_in *) info->ai_addr;
# if defined(HAVE_STRUCT_UTMPX_UT_ADDR)
				utent->ut_addr = sa->sin_addr.s_addr;
# endif
# if defined(HAVE_STRUCT_UTMPX_UT_ADDR_V6)
				utent->ut_addr_v6[0] = sa->sin_addr.s_addr;
			} else if (info->ai_family == AF_INET6) {
				struct sockaddr_in6 *sa =
					(struct sockaddr_in6 *) info->ai_addr;
				memcpy (utent->ut_addr_v6,
				        &(sa->sin6_addr),
				        MIN(sizeof(utent->ut_addr_v6),
				            sizeof(sa->sin6_addr)));
# endif
			}
			freeaddrinfo (info);
		}
#endif
		free (hostname);
	}
	/* ut_exit is only for DEAD_PROCESS */
	utent->ut_session = getsid (0);
	if (gettimeofday (&tv, NULL) == 0) {
#if defined(HAVE_STRUCT_UTMPX_UT_TIME)
		utent->ut_time = tv.tv_sec;
#endif
#if defined(HAVE_STRUCT_UTMPX_UT_XTIME)
		utent->ut_xtime = tv.tv_usec;
#endif
		utent->ut_tv.tv_sec  = tv.tv_sec;
		utent->ut_tv.tv_usec = tv.tv_usec;
	}

	return utent;
}


/*
 * setutmp - Update an entry in utmp and log an entry in wtmp
 *
 *	Return 1 on failure and 0 on success.
 */
static int
setutmp(struct utmpx *ut)
{
	int err = 0;

	assert (NULL != ut);

	setutxent();
	if (pututxline(ut) == NULL) {
		err = 1;
	}
	endutxent();

#if !defined(USE_PAM)
	/* This is done by pam_lastlog */
	updwtmpx(_PATH_WTMP, ut);
#endif

	return err;
}


int
update_utmp(const char *user, const char *tty, const char *host,
            pid_t main_pid)
{
	struct utmpx  *utent, *ut;

	utent = get_current_utmp(main_pid);
	ut = prepare_utmp(user, tty, host, utent, main_pid);

	(void) setutmp  (ut);	/* make entry in the utmp & wtmp files */

	free(utent);
	free(ut);

	return 0;
}


void
record_failure(const char *failent_user, const char *tty, const char *hostname,
               pid_t main_pid)
{
	struct utmpx  *utent, *failent;

	if (getdef_str ("FTMP_FILE") != NULL) {
		utent = get_current_utmp(main_pid);
		failent = prepare_utmp(failent_user, tty, hostname, utent,
		                       main_pid);
		failtmp (failent_user, failent);
		free (utent);
		free (failent);
	}
}


unsigned long
active_sessions_count(const char *name, unsigned long limit)
{
	struct utmpx   *ut;
	unsigned long  count = 0;

	setutxent();
	while ((ut = getutxent()))
	{
		if (USER_PROCESS != ut->ut_type) {
			continue;
		}
		if (strneq_a(ut->ut_user, ""))
			continue;

		if (!strneq_a(ut->ut_user, name))
			continue;

		count++;
		if (count > limit) {
			break;
		}
	}
	endutxent();

	return count;
}
