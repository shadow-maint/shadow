/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1999, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
 * Copyright (c) 2008       , Nicolas François
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

#if HAVE_UTMPX_H
#include <utmpx.h>
#endif

#include <fcntl.h>
#include <stdio.h>

#ident "$Id$"

#if HAVE_UTMPX_H
struct utmpx utxent;
#endif
struct utmp utent;

#define	NO_UTENT \
	_("No utmp entry.  You must exec \"login\" from the lowest level \"sh\"")
#define	NO_TTY \
	_("Unable to determine your tty name.")

/*
 * is_my_tty -- determine if "tty" is the same TTY stdin is using
 */
static bool is_my_tty (const char *tty)
{
	char full_tty[200];
	struct stat by_name, by_fd;

	if ('/' != *tty) {
		snprintf (full_tty, sizeof full_tty, "/dev/%s", tty);
		tty = full_tty;
	}

	if (   (stat (tty, &by_name) != 0)
	    || (fstat (STDIN_FILENO, &by_fd) != 0)) {
		return false;
	}

	if (by_name.st_rdev != by_fd.st_rdev) {
		return false;
	} else {
		return true;
	}
}

/*
 * checkutmp - see if utmp file is correct for this process
 *
 *	System V is very picky about the contents of the utmp file
 *	and requires that a slot for the current process exist.
 *	The utmp file is scanned for an entry with the same process
 *	ID.  If no entry exists the process exits with a message.
 *
 *	The "picky" flag is for network and other logins that may
 *	use special flags.  It allows the pid checks to be overridden.
 *	This means that getty should never invoke login with any
 *	command line flags.
 */

#if defined(__linux__)		/* XXX */

void checkutmp (bool picky)
{
	char *line;
	struct utmp *ut;
	pid_t pid = getpid ();

	setutent ();

	/* First, try to find a valid utmp entry for this process.  */
	while ((ut = getutent ()) != NULL) {
		if (   (ut->ut_pid == pid)
		    && ('\0' != ut->ut_line[0])
		    && ('\0' != ut->ut_id[0])
		    && (   (LOGIN_PROCESS == ut->ut_type)
		        || (USER_PROCESS  == ut->ut_type))
		    /* A process may have failed to close an entry
		     * Check if this entry refers to the current tty */
		    && is_my_tty (ut->ut_line)) {
			break;
		}
	}

	/* We cannot trust the ut_line field. Prepare the new value. */
	line = ttyname (0);
	if (NULL == line) {
		(void) puts (NO_TTY);
		exit (EXIT_FAILURE);
	}
	if (strncmp (line, "/dev/", 5) == 0) {
		line += 5;
	}

	/* If there is one, just use it, otherwise create a new one. */
	if (NULL != ut) {
		utent = *ut;
	} else {
		if (picky) {
			(void) puts (NO_UTENT);
			exit (EXIT_FAILURE);
		}
		memset ((void *) &utent, 0, sizeof utent);
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_pid = pid;
		/* XXX - assumes /dev/tty?? or /dev/pts/?? */
		strncpy (utent.ut_id, line + 3, sizeof utent.ut_id);
		strcpy (utent.ut_user, "LOGIN");
		utent.ut_time = time (NULL);
	}

	/* Sanitize / set the ut_line field */
	strncpy (utent.ut_line, line, sizeof utent.ut_line);
}

#elif defined(LOGIN_PROCESS)

void checkutmp (bool picky)
{
	char *line;
	struct utmp *ut;

#if HAVE_UTMPX_H
	struct utmpx *utx;
#endif
	pid_t pid = getpid ();

#if HAVE_UTMPX_H
	setutxent ();
#endif
	setutent ();

	if (picky) {
#if HAVE_UTMPX_H
		while ((utx = getutxent ()) != NULL) {
			if (utx->ut_pid == pid) {
				break;
			}
		}

		if (NULL != utx) {
			utxent = *utx;
		}
#endif
		while ((ut = getutent ()) != NULL) {
			if (ut->ut_pid == pid) {
				break;
			}
		}

		if (NULL != ut) {
			utent = *ut;
		}

#if HAVE_UTMPX_H
		endutxent ();
#endif
		endutent ();

		if (NULL == ut) {
			(void) puts (NO_UTENT);
			exit (EXIT_FAILURE);
		}
#ifndef	UNIXPC

		/*
		 * If there is no ut_line value in this record, fill
		 * it in by getting the TTY name and stuffing it in
		 * the structure.  The UNIX/PC is broken in this regard
		 * and needs help ...
		 */
		/* XXX: The ut_line may not match with the current tty.
		 *      ut_line will be set by setutmp anyway, but ut_id
		 *      will not be set, and the wrong utmp entry may be
		 *      updated. -- nekral */

		if (utent.ut_line[0] == '\0')
#endif				/* !UNIXPC */
		{
			line = ttyname (0);
			if (NULL == line) {
				(void) puts (NO_TTY);
				exit (EXIT_FAILURE);
			}
			if (strncmp (line, "/dev/", 5) == 0) {
				line += 5;
			}
			strncpy (utent.ut_line, line, sizeof utent.ut_line);
#if HAVE_UTMPX_H
			strncpy (utxent.ut_line, line, sizeof utxent.ut_line);
#endif
		}
	} else {
		line = ttyname (0);
		if (NULL == line) {
			(void) puts (NO_TTY);
			exit (EXIT_FAILURE);
		}
		if (strncmp (line, "/dev/", 5) == 0) {
			line += 5;
		}

		strncpy (utent.ut_line, line, sizeof utent.ut_line);
		ut = getutline (&utent);
		if (NULL != ut) {
			strncpy (utent.ut_id, ut->ut_id, sizeof ut->ut_id);
		}

		strcpy (utent.ut_user, "LOGIN");
		utent.ut_pid = getpid ();
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_time = time (NULL);
#if HAVE_UTMPX_H
		strncpy (utxent.ut_line, line, sizeof utxent.ut_line);
		utx = getutxline (&utxent);
		if (NULL != utx) {
			strncpy (utxent.ut_id, utx->ut_id, sizeof utxent.ut_id);
		}

		strcpy (utxent.ut_user, "LOGIN");
		utxent.ut_pid = utent.ut_pid;
		utxent.ut_type = utent.ut_type;
		if (sizeof (utxent.ut_tv) == sizeof (struct timeval)) {
			gettimeofday ((struct timeval *) &utxent.ut_tv, NULL);
		} else {
			struct timeval tv;

			gettimeofday (&tv, NULL);
			utxent.ut_tv.tv_sec = tv.tv_sec;
			utxent.ut_tv.tv_usec = tv.tv_usec;
		}
		utent.ut_time = utxent.ut_tv.tv_sec;
#endif
	}
}

#endif


/*
 * Some systems already have updwtmp() and possibly updwtmpx().  Others
 * don't, so we re-implement these functions if necessary.  --marekm
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

#ifdef HAVE_UTMPX_H
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
#endif				/* ! HAVE_UTMPX_H */


/*
 * setutmp - put a USER_PROCESS entry in the utmp file
 *
 *	setutmp changes the type of the current utmp entry to
 *	USER_PROCESS.  the wtmp file will be updated as well.
 */

#if defined(__linux__)		/* XXX */

int setutmp (const char *name, const char unused(*line), const char unused(*host))
{
	int err = 0;
	utent.ut_type = USER_PROCESS;
	strncpy (utent.ut_user, name, sizeof utent.ut_user);
	utent.ut_time = time (NULL);
	/* other fields already filled in by checkutmp above */
	setutent ();
	if (pututline (&utent) == NULL) {
		err = 1;
	}
	endutent ();
	updwtmp (_WTMP_FILE, &utent);

	return err;
}

#elif HAVE_UTMPX_H

int setutmp (const char *name, const char *line, const char *host)
{
	struct utmp *utmp, utline;
	struct utmpx *utmpx, utxline;
	pid_t pid = getpid ();
	bool found_utmpx = false;
	bool found_utmp = false;
	int err = 0;

	/*
	 * The canonical device name doesn't include "/dev/"; skip it
	 * if it is already there.
	 */

	if (strncmp (line, "/dev/", 5) == 0) {
		line += 5;
	}

	/*
	 * Update utmpx.  We create an empty entry in case there is
	 * no matching entry in the utmpx file.
	 */

	setutxent ();
	setutent ();

	while ((utmpx = getutxent ()) != NULL) {
		if (utmpx->ut_pid == pid) {
			found_utmpx = true;
			break;
		}
	}
	while ((utmp = getutent ()) != NULL) {
		if (utmp->ut_pid == pid) {
			found_utmp = true;
			break;
		}
	}

	/*
	 * If the entry matching `pid' cannot be found, create a new
	 * entry with the device name in it.
	 */

	if (!found_utmpx) {
		memset ((void *) &utxline, 0, sizeof utxline);
		strncpy (utxline.ut_line, line, sizeof utxline.ut_line);
		utxline.ut_pid = getpid ();
	} else {
		utxline = *utmpx;
		if (strncmp (utxline.ut_line, "/dev/", 5) == 0) {
			memmove (utxline.ut_line, utxline.ut_line + 5,
				 sizeof utxline.ut_line - 5);
			utxline.ut_line[sizeof utxline.ut_line - 5] = '\0';
		}
	}
	if (!found_utmp) {
		memset ((void *) &utline, 0, sizeof utline);
		strncpy (utline.ut_line, utxline.ut_line,
			 sizeof utline.ut_line);
		utline.ut_pid = utxline.ut_pid;
	} else {
		utline = *utmp;
		if (strncmp (utline.ut_line, "/dev/", 5) == 0) {
			memmove (utline.ut_line, utline.ut_line + 5,
				 sizeof utline.ut_line - 5);
			utline.ut_line[sizeof utline.ut_line - 5] = '\0';
		}
	}

	/*
	 * Fill in the fields in the utmpx entry and write it out.  Do
	 * the utmp entry at the same time to make sure things don't
	 * get messed up.
	 */

	strncpy (utxline.ut_user, name, sizeof utxline.ut_user);
	strncpy (utline.ut_user, name, sizeof utline.ut_user);

	utline.ut_type = utxline.ut_type = USER_PROCESS;

	if (sizeof (utxline.ut_tv) == sizeof (struct timeval)) {
		gettimeofday ((struct timeval *) &utxline.ut_tv, NULL);
	} else {
		struct timeval tv;

		gettimeofday (&tv, NULL);
		utxline.ut_tv.tv_sec = tv.tv_sec;
		utxline.ut_tv.tv_usec = tv.tv_usec;
	}
	utline.ut_time = utxline.ut_tv.tv_sec;

	strncpy (utxline.ut_host, (NULL != host) ? host : "",
	         sizeof utxline.ut_host);

	if (   (pututxline (&utxline) == NULL)
	    || (pututline (&utline) == NULL)) {
		err = 1;
	}

	updwtmpx (_WTMP_FILE "x", &utxline);
	updwtmp (_WTMP_FILE, &utline);

	utxent = utxline;
	utent = utline;

	return err;
}

#endif
