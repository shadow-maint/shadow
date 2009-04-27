/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1999, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
 * Copyright (c) 2008 - 2009, Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include "defines.h"
#include "prototypes.h"

#include <utmp.h>

#if USE_UTMPX
#include <utmpx.h>
#endif

#include <assert.h>
#include <netdb.h>
#include <stdio.h>

#ident "$Id$"


/*
 * is_my_tty -- determine if "tty" is the same TTY stdin is using
 */
static bool is_my_tty (const char *tty)
{
	/* full_tty shall be at least sizeof utmp.ut_line + 5 */
	char full_tty[200];
	/* tmptty shall be bigger than full_tty */
	static char tmptty[sizeof (full_tty)+1];

	if ('/' != *tty) {
		(void) snprintf (full_tty, sizeof full_tty, "/dev/%s", tty);
		tty = &full_tty[0];
	}

	if ('\0' == tmptty[0]) {
		const char *tname = ttyname (STDIN_FILENO);
		if (NULL != tname) {
			(void) strncpy (tmptty, tname, sizeof tmptty);
			tmptty[sizeof (tmptty) - 1] = '\0';
		}
	}

	if (NULL == tmptty) {
		(void) puts (_("Unable to determine your tty name."));
		exit (EXIT_FAILURE);
	} else if (strncmp (tty, tmptty, sizeof (tmptty)) != 0) {
		return false;
	} else {
		return true;
	}
}

/*
 * get_current_utmp - return the most probable utmp entry for the current
 *                    session
 *
 *	The utmp file is scanned for an entry with the same process ID.
 *	The line enterred by the *getty / telnetd, etc. should also match
 *	the current terminal.
 *
 *	When an entry is returned by get_current_utmp, and if the utmp
 *	structure has a ut_id field, this field should be used to update
 *	the entry information.
 *
 *	Return NULL if no entries exist in utmp for the current process.
 */
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
		ret = (struct utmp *) xmalloc (sizeof (*ret));
		memcpy (ret, ut, sizeof (*ret));
	}

	endutent ();

	return ret;
}

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
		write (fd, (const char *) ut, sizeof (*ut));
		close (fd);
	}
}
#endif				/* ! HAVE_UPDWTMP */

#ifdef USE_UTMPX
#ifndef HAVE_UPDWTMPX
static void updwtmpx (const char *filename, const struct utmpx *utx)
{
	int fd;

	fd = open (filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write (fd, (const char *) utx, sizeof (*utx));
		close (fd);
	}
}
#endif				/* ! HAVE_UPDWTMPX */
#endif				/* ! USE_UTMPX */


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
		hostname = (char *) xmalloc (strlen (host) + 1);
		strcpy (hostname, host);
#ifdef HAVE_STRUCT_UTMP_UT_HOST
	} else if (   (NULL != ut)
	           && (NULL != ut->ut_host)
	           && ('\0' != ut->ut_host[0])) {
		hostname = (char *) xmalloc (sizeof (ut->ut_host) + 1);
		strncpy (hostname, ut->ut_host, sizeof (ut->ut_host));
		hostname[sizeof (ut->ut_host)] = '\0';
#endif				/* HAVE_STRUCT_UTMP_UT_HOST */
	}

	if (strncmp(line, "/dev/", 5) == 0) {
		line += 5;
	}


	utent = (struct utmp *) xmalloc (sizeof (*utent));
	memzero (utent, sizeof (*utent));



#ifdef HAVE_STRUCT_UTMP_UT_TYPE
	utent->ut_type = USER_PROCESS;
#endif				/* HAVE_STRUCT_UTMP_UT_TYPE */
	utent->ut_pid = getpid ();
	strncpy (utent->ut_line, line,      sizeof (utent->ut_line));
#ifdef HAVE_STRUCT_UTMP_UT_ID
	if (NULL != ut) {
		strncpy (utent->ut_id, ut->ut_id, sizeof (utent->ut_id));
	} else {
		/* XXX - assumes /dev/tty?? */
		strncpy (utent->ut_id, line + 3, sizeof (utent->ut_id));
	}
#endif				/* HAVE_STRUCT_UTMP_UT_ID */
#ifdef HAVE_STRUCT_UTMP_UT_NAME
	strncpy (utent->ut_name, name,      sizeof (utent->ut_name));
#endif				/* HAVE_STRUCT_UTMP_UT_NAME */
#ifdef HAVE_STRUCT_UTMP_UT_USER
	strncpy (utent->ut_user, name,      sizeof (utent->ut_user));
#endif				/* HAVE_STRUCT_UTMP_UT_USER */
	if (NULL != hostname) {
		struct addrinfo *info = NULL;
#ifdef HAVE_STRUCT_UTMP_UT_HOST
		strncpy (utent->ut_host, hostname, sizeof (utent->ut_host));
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
int setutmp (struct utmp *ut)
{
	int err = 0;

	assert (NULL != ut);

	setutent ();
	if (pututline (ut) == NULL) {
		err = 1;
	}
	endutent ();

	updwtmp (_WTMP_FILE, ut);

	return err;
}

#ifdef USE_UTMPX
/*
 * prepare_utmpx - the UTMPX version for prepare_utmp
 */
/*@only@*/struct utmpx *prepare_utmpx (const char *name,
                                       const char *line,
                                       const char *host,
                                       /*@null@*/const struct utmp *ut)
{
	struct timeval tv;
	char *hostname = NULL;
	struct utmpx *utxent;

	assert (NULL != name);
	assert (NULL != line);



	if (   (NULL != host)
	    && ('\0' != host[0])) {
		hostname = (char *) xmalloc (strlen (host) + 1);
		strcpy (hostname, host);
#ifdef HAVE_STRUCT_UTMP_UT_HOST
	} else if (   (NULL != ut)
	           && (NULL != ut->ut_host)
	           && ('\0' != ut->ut_host[0])) {
		hostname = (char *) xmalloc (sizeof (ut->ut_host) + 1);
		strncpy (hostname, ut->ut_host, sizeof (ut->ut_host));
		hostname[sizeof (ut->ut_host)] = '\0';
#endif				/* HAVE_STRUCT_UTMP_UT_TYPE */
	}

	if (strncmp(line, "/dev/", 5) == 0) {
		line += 5;
	}

	utxent = (struct utmpx *) xmalloc (sizeof (*utxent));
	memzero (utxent, sizeof (*utxent));



	utxent->ut_type = USER_PROCESS;
	utxent->ut_pid = getpid ();
	strncpy (utxent->ut_line, line,      sizeof (utxent->ut_line));
	/* existence of ut->ut_id is enforced by configure */
	if (NULL != ut) {
		strncpy (utxent->ut_id, ut->ut_id, sizeof (utxent->ut_id));
	} else {
		/* XXX - assumes /dev/tty?? */
		strncpy (utxent->ut_id, line + 3, sizeof (utxent->ut_id));
	}
#ifdef HAVE_STRUCT_UTMPX_UT_NAME
	strncpy (utxent->ut_name, name,      sizeof (utxent->ut_name));
#endif				/* HAVE_STRUCT_UTMPX_UT_NAME */
	strncpy (utxent->ut_user, name,      sizeof (utxent->ut_user));
	if (NULL != hostname) {
		struct addrinfo *info = NULL;
#ifdef HAVE_STRUCT_UTMPX_UT_HOST
		strncpy (utxent->ut_host, hostname, sizeof (utxent->ut_host));
#endif				/* HAVE_STRUCT_UTMPX_UT_HOST */
#ifdef HAVE_STRUCT_UTMPX_UT_SYSLEN
		utxent->ut_syslen = MIN (strlen (hostname),
		                         sizeof (utxent->ut_host));
#endif				/* HAVE_STRUCT_UTMPX_UT_SYSLEN */
#if defined(HAVE_STRUCT_UTMPX_UT_ADDR) || defined(HAVE_STRUCT_UTMPX_UT_ADDR_V6)
		if (getaddrinfo (hostname, NULL, NULL, &info) == 0) {
			/* getaddrinfo might not be reliable.
			 * Just try to log what may be useful.
			 */
			if (info->ai_family == AF_INET) {
				struct sockaddr_in *sa =
					(struct sockaddr_in *) info->ai_addr;
#ifdef HAVE_STRUCT_UTMPX_UT_ADDR
				memcpy (utxent->ut_addr,
				        &(sa->sin_addr),
				        MIN (sizeof (utxent->ut_addr),
				             sizeof (sa->sin_addr)));
#endif				/* HAVE_STRUCT_UTMPX_UT_ADDR */
#ifdef HAVE_STRUCT_UTMPX_UT_ADDR_V6
				memcpy (utxent->ut_addr_v6,
				        &(sa->sin_addr),
				        MIN (sizeof (utxent->ut_addr_v6),
				             sizeof (sa->sin_addr)));
			} else if (info->ai_family == AF_INET6) {
				struct sockaddr_in6 *sa =
					(struct sockaddr_in6 *) info->ai_addr;
				memcpy (utxent->ut_addr_v6,
				        &(sa->sin6_addr),
				        MIN (sizeof (utxent->ut_addr_v6),
				             sizeof (sa->sin6_addr)));
#endif				/* HAVE_STRUCT_UTMPX_UT_ADDR_V6 */
			}
			freeaddrinfo (info);
		}
#endif		/* HAVE_STRUCT_UTMPX_UT_ADDR || HAVE_STRUCT_UTMPX_UT_ADDR_V6 */
		free (hostname);
	}
	/* ut_exit is only for DEAD_PROCESS */
	utxent->ut_session = getsid (0);
	if (gettimeofday (&tv, NULL) == 0) {
#ifdef HAVE_STRUCT_UTMPX_UT_TIME
		utxent->ut_time = tv.tv_sec;
#endif				/* HAVE_STRUCT_UTMPX_UT_TIME */
#ifdef HAVE_STRUCT_UTMPX_UT_XTIME
		utxent->ut_xtime = tv.tv_usec;
#endif				/* HAVE_STRUCT_UTMPX_UT_XTIME */
		utxent->ut_tv.tv_sec  = tv.tv_sec;
		utxent->ut_tv.tv_usec = tv.tv_usec;
	}

	return utxent;
}

/*
 * setutmpx - the UTMPX version for setutmp
 */
int setutmpx (struct utmpx *utx)
{
	int err = 0;

	assert (NULL != utx);

	setutxent ();
	if (pututxline (utx) == NULL) {
		err = 1;
	}
	endutxent ();

	updwtmpx (_WTMP_FILE "x", utx);

	return err;
}
#endif				/* USE_UTMPX */

