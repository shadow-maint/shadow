/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "defines.h"

#include <utmp.h>

#if HAVE_UTMPX_H
#include <utmpx.h>
#endif

#include <fcntl.h>
#include <stdio.h>

#include "rcsid.h"
RCSID ("$Id: utmp.c,v 1.14 2003/12/17 12:52:25 kloczek Exp $")
#if HAVE_UTMPX_H
struct utmpx utxent;
#endif
struct utmp utent;

extern struct utmp *getutent ();
extern struct utmp *getutline ();
extern void setutent ();
extern void endutent ();

#define	NO_UTENT \
	"No utmp entry.  You must exec \"login\" from the lowest level \"sh\""
#define	NO_TTY \
	"Unable to determine your tty name."

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

void checkutmp (int picky)
{
	char *line;
	struct utmp *ut;
	pid_t pid = getpid ();

	setutent ();

	/* First, try to find a valid utmp entry for this process.  */
	while ((ut = getutent ()))
		if (ut->ut_pid == pid && ut->ut_line[0] && ut->ut_id[0] &&
		    (ut->ut_type == LOGIN_PROCESS
		     || ut->ut_type == USER_PROCESS))
			break;

	/* If there is one, just use it, otherwise create a new one.  */
	if (ut) {
		utent = *ut;
	} else {
		if (picky) {
			puts (NO_UTENT);
			exit (1);
		}
		line = ttyname (0);
		if (!line) {
			puts (NO_TTY);
			exit (1);
		}
		if (strncmp (line, "/dev/", 5) == 0)
			line += 5;
		memset ((void *) &utent, 0, sizeof utent);
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_pid = pid;
		strncpy (utent.ut_line, line, sizeof utent.ut_line);
		/* XXX - assumes /dev/tty?? */
		strncpy (utent.ut_id, utent.ut_line + 3,
			 sizeof utent.ut_id);
		strcpy (utent.ut_user, "LOGIN");
		utent.ut_time = time (NULL);
	}
}

#elif defined(LOGIN_PROCESS)

void checkutmp (int picky)
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
		while ((utx = getutxent ()))
			if (utx->ut_pid == pid)
				break;

		if (utx)
			utxent = *utx;
#endif
		while ((ut = getutent ()))
			if (ut->ut_pid == pid)
				break;

		if (ut)
			utent = *ut;

#if HAVE_UTMPX_H
		endutxent ();
#endif
		endutent ();

		if (!ut) {
			puts (NO_UTENT);
			exit (1);
		}
#ifndef	UNIXPC

		/*
		 * If there is no ut_line value in this record, fill
		 * it in by getting the TTY name and stuffing it in
		 * the structure.  The UNIX/PC is broken in this regard
		 * and needs help ...
		 */

		if (utent.ut_line[0] == '\0')
#endif				/* !UNIXPC */
		{
			if (!(line = ttyname (0))) {
				puts (NO_TTY);
				exit (1);
			}
			if (strncmp (line, "/dev/", 5) == 0)
				line += 5;
			strncpy (utent.ut_line, line,
				 sizeof utent.ut_line);
#if HAVE_UTMPX_H
			strncpy (utxent.ut_line, line,
				 sizeof utxent.ut_line);
#endif
		}
	} else {
		if (!(line = ttyname (0))) {
			puts (NO_TTY);
			exit (1);
		}
		if (strncmp (line, "/dev/", 5) == 0)
			line += 5;

		strncpy (utent.ut_line, line, sizeof utent.ut_line);
		if ((ut = getutline (&utent)))
			strncpy (utent.ut_id, ut->ut_id, sizeof ut->ut_id);

		strcpy (utent.ut_user, "LOGIN");
		utent.ut_pid = getpid ();
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_time = time (NULL);
#if HAVE_UTMPX_H
		strncpy (utxent.ut_line, line, sizeof utxent.ut_line);
		if ((utx = getutxline (&utxent)))
			strncpy (utxent.ut_id, utx->ut_id,
				 sizeof utxent.ut_id);

		strcpy (utxent.ut_user, "LOGIN");
		utxent.ut_pid = utent.ut_pid;
		utxent.ut_type = utent.ut_type;
		if (sizeof (utxent.ut_tv) == sizeof (struct timeval))
			gettimeofday ((struct timeval *) &utxent.ut_tv,
				      NULL);
		else {
			struct timeval tv;

			gettimeofday (&tv, NULL);
			utxent.ut_tv.tv_sec = tv.tv_sec;
			utxent.ut_tv.tv_usec = tv.tv_usec;
		}
		utent.ut_time = utxent.ut_tv.tv_sec;
#endif
	}
}

#else				/* !USG */

void checkutmp (int picky)
{
	char *line;

	/*
	 * Hand-craft a new utmp entry.
	 */

	memzero (&utent, sizeof utent);
	if (!(line = ttyname (0))) {
		puts (NO_TTY);
		exit (1);
	}
	if (strncmp (line, "/dev/", 5) == 0)
		line += 5;

	(void) strncpy (utent.ut_line, line, sizeof utent.ut_line);
	utent.ut_time = time (NULL);
}

#endif				/* !USG */


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

void setutmp (const char *name, const char *line, const char *host)
{
	utent.ut_type = USER_PROCESS;
	strncpy (utent.ut_user, name, sizeof utent.ut_user);
	utent.ut_time = time (NULL);
	/* other fields already filled in by checkutmp above */
	setutent ();
	pututline (&utent);
	endutent ();
	updwtmp (_WTMP_FILE, &utent);
}

#elif HAVE_UTMPX_H

void setutmp (const char *name, const char *line, const char *host)
{
	struct utmp *utmp, utline;
	struct utmpx *utmpx, utxline;
	pid_t pid = getpid ();
	int found_utmpx = 0, found_utmp = 0;

	/*
	 * The canonical device name doesn't include "/dev/"; skip it
	 * if it is already there.
	 */

	if (strncmp (line, "/dev/", 5) == 0)
		line += 5;

	/*
	 * Update utmpx.  We create an empty entry in case there is
	 * no matching entry in the utmpx file.
	 */

	setutxent ();
	setutent ();

	while (utmpx = getutxent ()) {
		if (utmpx->ut_pid == pid) {
			found_utmpx = 1;
			break;
		}
	}
	while (utmp = getutent ()) {
		if (utmp->ut_pid == pid) {
			found_utmp = 1;
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

	if (sizeof (utxline.ut_tv) == sizeof (struct timeval))
		gettimeofday ((struct timeval *) &utxline.ut_tv, NULL);
	else {
		struct timeval tv;

		gettimeofday (&tv, NULL);
		utxline.ut_tv.tv_sec = tv.tv_sec;
		utxline.ut_tv.tv_usec = tv.tv_usec;
	}
	utline.ut_time = utxline.ut_tv.tv_sec;

	strncpy (utxline.ut_host, host ? host : "",
		 sizeof utxline.ut_host);

	pututxline (&utxline);
	pututline (&utline);

	updwtmpx (_WTMP_FILE "x", &utxline);
	updwtmp (_WTMP_FILE, &utline);

	utxent = utxline;
	utent = utline;
}

#endif
