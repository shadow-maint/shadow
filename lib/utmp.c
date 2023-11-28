/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#include "defines.h"
#include "prototypes.h"
#include "getdef.h"

#include <utmp.h>
#include <assert.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>

#include "alloc.h"
#include "sizeof.h"
#include "string/strncpy.h"
#include "string/strtcpy.h"
#include "string/zustr2stp.h"

#ident "$Id$"


/*
 * is_my_tty -- determine if "tty" is the same TTY stdin is using
 */
static bool is_my_tty (const char tty[UT_LINESIZE])
{
	char         full_tty[STRLEN("/dev/") + UT_LINESIZE + 1];
	/* tmptty shall be bigger than full_tty */
	static char  tmptty[sizeof(full_tty) + 1];

	full_tty[0] = '\0';
	if (tty[0] != '/')
		strcpy (full_tty, "/dev/");
	strncat (full_tty, tty, UT_LINESIZE);

	if ('\0' == tmptty[0]) {
		const char *tname = ttyname (STDIN_FILENO);
		if (NULL != tname)
			STRTCPY(tmptty, tname);
	}

	if ('\0' == tmptty[0]) {
		(void) puts (_("Unable to determine your tty name."));
		exit (EXIT_FAILURE);
	}

	return strcmp (full_tty, tmptty) == 0;
}

/*
 * failtmp - update the cumulative failure log
 *
 *	failtmp updates the (struct utmp) formatted failure log which
 *	maintains a record of all login failures.
 */
static void failtmp (const char *username, const struct utmp *failent)
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

	if (access (ftmp, F_OK) != 0) {
		return;
	}

	fd = open (ftmp, O_WRONLY | O_APPEND);
	if (-1 == fd) {
		SYSLOG ((LOG_WARN,
		         "Can't append failure of user %s to %s.",
		         username, ftmp));
		return;
	}

	/*
	 * Append the new failure record and close the log file.
	 */

	if (   (write_full(fd, failent, sizeof *failent) == -1)
	    || (close (fd) != 0)) {
		SYSLOG ((LOG_WARN,
		         "Can't append failure of user %s to %s.",
		         username, ftmp));
		(void) close (fd);
	}
}

/*
 * get_current_utmp - return the most probable utmp entry for the current
 *                    session
 *
 *	The utmp file is scanned for an entry with the same process ID.
 *	The line entered by the *getty / telnetd, etc. should also match
 *	the current terminal.
 *
 *	When an entry is returned by get_current_utmp, and if the utmp
 *	structure has a ut_id field, this field should be used to update
 *	the entry information.
 *
 *	Return NULL if no entries exist in utmp for the current process.
 */
static
/*@null@*/ /*@only@*/struct utmp *get_current_utmp (void)
{
	struct utmp *ut;
	struct utmp *ret = NULL;

	setutent ();

	/* First, try to find a valid utmp entry for this process.  */
	while ((ut = getutent ()) != NULL) {
		if (   (ut->ut_pid == getpid ())
#ifdef HAVE_STRUCT_UTMP_UT_ID
		    && ('\0' != ut->ut_id[0])
#endif
#ifdef HAVE_STRUCT_UTMP_UT_TYPE
		    && (   (LOGIN_PROCESS == ut->ut_type)
		        || (USER_PROCESS  == ut->ut_type))
#endif
		    /* A process may have failed to close an entry
		     * Check if this entry refers to the current tty */
		    && is_my_tty (ut->ut_line)) {
			break;
		}
	}

	if (NULL != ut) {
		ret = XMALLOC(1, struct utmp);
		memcpy (ret, ut, sizeof (*ret));
	}

	endutent ();

	return ret;
}

int get_session_host (char **out)
{
	char *hostname = NULL;
	struct utmp *ut = NULL;
	int ret = 0;

	ut = get_current_utmp();

#ifdef HAVE_STRUCT_UTMP_UT_HOST
	if ((ut != NULL) && (ut->ut_host[0] != '\0')) {
		hostname = XMALLOC(sizeof(ut->ut_host) + 1, char);
		ZUSTR2STP(hostname, ut->ut_host);
		*out = hostname;
		free (ut);
	} else {
		*out = NULL;
		ret = -2;
	}
#else
	*out = NULL;
	ret = -2;
#endif /* HAVE_STRUCT_UTMP_UT_HOST */

	return ret;
}

#ifndef USE_PAM
/*
 * Some systems already have updwtmp() and possibly updwtmpx().  Others
 * don't, so we re-implement these functions if necessary.
 */
#ifndef HAVE_UPDWTMP
static void updwtmp (const char *filename, const struct utmp *ut)
{
	int fd;

	fd = open (filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write_full(fd, ut, sizeof(*ut));
		close (fd);
	}
}
#endif				/* ! HAVE_UPDWTMP */

#endif				/* ! USE_PAM */


/*
 * prepare_utmp - prepare an utmp entry so that it can be logged in a
 *                utmp/wtmp file.
 *
 *	It accepts an utmp entry in input (ut) to return an entry with
 *	the right ut_id. This is typically an entry returned by
 *	get_current_utmp
 *	If ut is NULL, ut_id will be forged based on the line argument.
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
static
/*@only@*/struct utmp *prepare_utmp (const char *name,
                                     const char *line,
                                     const char *host,
                                     /*@null@*/const struct utmp *ut)
{
	struct timeval tv;
	char *hostname = NULL;
	struct utmp *utent;

	assert (NULL != name);
	assert (NULL != line);



	if (   (NULL != host)
	    && ('\0' != host[0])) {
		hostname = XMALLOC(strlen(host) + 1, char);
		strcpy (hostname, host);
#ifdef HAVE_STRUCT_UTMP_UT_HOST
	} else if (   (NULL != ut)
	           && ('\0' != ut->ut_host[0])) {
		hostname = XMALLOC(NITEMS(ut->ut_host) + 1, char);
		ZUSTR2STP(hostname, ut->ut_host);
#endif				/* HAVE_STRUCT_UTMP_UT_HOST */
	}

	if (strncmp(line, "/dev/", 5) == 0) {
		line += 5;
	}


	utent = XCALLOC (1, struct utmp);


#ifdef HAVE_STRUCT_UTMP_UT_TYPE
	utent->ut_type = USER_PROCESS;
#endif				/* HAVE_STRUCT_UTMP_UT_TYPE */
	utent->ut_pid = getpid ();
	STRNCPY(utent->ut_line, line);
#ifdef HAVE_STRUCT_UTMP_UT_ID
	if (NULL != ut) {
		STRNCPY(utent->ut_id, ut->ut_id);
	} else {
		/* XXX - assumes /dev/tty?? */
		STRNCPY(utent->ut_id, line + 3);
	}
#endif				/* HAVE_STRUCT_UTMP_UT_ID */
#ifdef HAVE_STRUCT_UTMP_UT_NAME
	STRNCPY(utent->ut_name, name);
#endif				/* HAVE_STRUCT_UTMP_UT_NAME */
#ifdef HAVE_STRUCT_UTMP_UT_USER
	STRNCPY(utent->ut_user, name);
#endif				/* HAVE_STRUCT_UTMP_UT_USER */
	if (NULL != hostname) {
		struct addrinfo *info = NULL;
#ifdef HAVE_STRUCT_UTMP_UT_HOST
		STRNCPY(utent->ut_host, hostname);
#endif				/* HAVE_STRUCT_UTMP_UT_HOST */
#ifdef HAVE_STRUCT_UTMP_UT_SYSLEN
		utent->ut_syslen = MIN (strlen (hostname),
		                        sizeof (utent->ut_host));
#endif				/* HAVE_STRUCT_UTMP_UT_SYSLEN */
#if defined(HAVE_STRUCT_UTMP_UT_ADDR) || defined(HAVE_STRUCT_UTMP_UT_ADDR_V6)
		if (getaddrinfo (hostname, NULL, NULL, &info) == 0) {
			/* getaddrinfo might not be reliable.
			 * Just try to log what may be useful.
			 */
			if (info->ai_family == AF_INET) {
				struct sockaddr_in *sa =
					(struct sockaddr_in *) info->ai_addr;
#ifdef HAVE_STRUCT_UTMP_UT_ADDR
				memcpy (&(utent->ut_addr),
				        &(sa->sin_addr),
				        MIN (sizeof (utent->ut_addr),
				             sizeof (sa->sin_addr)));
#endif				/* HAVE_STRUCT_UTMP_UT_ADDR */
#ifdef HAVE_STRUCT_UTMP_UT_ADDR_V6
				memcpy (utent->ut_addr_v6,
				        &(sa->sin_addr),
				        MIN (sizeof (utent->ut_addr_v6),
				             sizeof (sa->sin_addr)));
			} else if (info->ai_family == AF_INET6) {
				struct sockaddr_in6 *sa =
					(struct sockaddr_in6 *) info->ai_addr;
				memcpy (utent->ut_addr_v6,
				        &(sa->sin6_addr),
				        MIN (sizeof (utent->ut_addr_v6),
				             sizeof (sa->sin6_addr)));
#endif				/* HAVE_STRUCT_UTMP_UT_ADDR_V6 */
			}
			freeaddrinfo (info);
		}
#endif		/* HAVE_STRUCT_UTMP_UT_ADDR || HAVE_STRUCT_UTMP_UT_ADDR_V6 */
		free (hostname);
	}
	/* ut_exit is only for DEAD_PROCESS */
	utent->ut_session = getsid (0);
	if (gettimeofday (&tv, NULL) == 0) {
#ifdef HAVE_STRUCT_UTMP_UT_TIME
		utent->ut_time = tv.tv_sec;
#endif				/* HAVE_STRUCT_UTMP_UT_TIME */
#ifdef HAVE_STRUCT_UTMP_UT_XTIME
		utent->ut_xtime = tv.tv_usec;
#endif				/* HAVE_STRUCT_UTMP_UT_XTIME */
#ifdef HAVE_STRUCT_UTMP_UT_TV
		utent->ut_tv.tv_sec  = tv.tv_sec;
		utent->ut_tv.tv_usec = tv.tv_usec;
#endif				/* HAVE_STRUCT_UTMP_UT_TV */
	}

	return utent;
}

/*
 * setutmp - Update an entry in utmp and log an entry in wtmp
 *
 *	Return 1 on failure and 0 on success.
 */
static int setutmp (struct utmp *ut)
{
	int err = 0;

	assert (NULL != ut);

	setutent ();
	if (pututline (ut) == NULL) {
		err = 1;
	}
	endutent ();

#ifndef USE_PAM
	/* This is done by pam_lastlog */
	updwtmp (_WTMP_FILE, ut);
#endif				/* ! USE_PAM */

	return err;
}

int update_utmp (const char *user,
                 const char *tty,
                 const char *host)
{
	struct utmp *utent, *ut;

	utent = get_current_utmp ();
	ut = prepare_utmp  (user, tty, host, utent);

	(void) setutmp  (ut);	/* make entry in the utmp & wtmp files */

	free(utent);
	free(ut);

	return 0;
}

void record_failure(const char *failent_user,
                    const char *tty,
                    const char *hostname)
{
	struct utmp *utent, *failent;

	if (getdef_str ("FTMP_FILE") != NULL) {
		utent = get_current_utmp ();
		failent = prepare_utmp (failent_user, tty, hostname, utent);
		failtmp (failent_user, failent);
		free (utent);
		free (failent);
	}
}

unsigned long active_sessions_count(const char *name, unsigned long limit)
{
	struct utmp *ut;
	unsigned long count = 0;

	setutent ();
	while ((ut = getutent ()))
	{
		if (USER_PROCESS != ut->ut_type) {
			continue;
		}
		if ('\0' == ut->ut_user[0]) {
			continue;
		}
		if (strncmp (name, ut->ut_user, sizeof (ut->ut_user)) != 0) {
			continue;
		}
		count++;
		if (count > limit) {
			break;
		}
	}
	endutent ();

	return count;
}
