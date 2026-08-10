/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#ifdef	SVR4
#include <stdlib.h>
#include <utmp.h>
#include <utmpx.h>
#include <sys/time.h>
#else
#include <sys/types.h>
#include <utmp.h>
#endif	/* SVR4 */

#include <fcntl.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#define	bzero(a,n)	memset(a, 0, n)
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include <stdio.h>
#ifdef	STDLIB_H
#include <stdlib.h>
#endif
#ifdef	UNISTD_H
#include <unistd.h>
#endif
#include "config.h"

#ifndef	UTMP_FILE
#define	UTMP_FILE	"/etc/utmp"
#endif

#if defined(SUN) || defined(BSD) || defined(SUN4)
#ifndef	WTMP_FILE
#define WTMP_FILE "/usr/adm/wtmp"
#endif
#endif	/* SUN || BSD */

#ifndef	lint
static	char	sccsid[] = "@(#)utmp.c	3.17	08:15:06	07 May 1993";
#endif

#ifdef	SVR4
extern	struct	utmpx	utxent;
extern	char	host;
#endif
extern	struct	utmp	utent;

extern	struct	utmp	*getutent();
extern	struct	utmp	*getutline();
extern	void	setutent();
extern	void	endutent();
extern	time_t	time();
extern	char	*ttyname();
extern	long	lseek();

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

void
checkutmp (picky)
int	picky;
{
	char	*line;
#ifdef	USG
#ifdef	SVR4
	struct	utmpx	*utx, utmpx, *getutxline();
#endif	/* SVR4 */
	struct	utmp	*ut, utmp, *getutline();
#ifndef	NDEBUG
	int	pid = getppid ();
#else
	int	pid = getpid ();
#endif	/* !NDEBUG */
#endif	/* USG */

#if !defined(SUN) && !defined(SUN4)
#ifdef	SVR4
	setutxent ();
#endif
	setutent ();
#endif	/* !SUN */

#ifdef	USG
	if (picky) {
#ifdef	SVR4
		while (utx = getutxent ())
			if (utx->ut_pid == pid)
				break;

		if (utx)
			utxent = *utx;
#endif
		while (ut = getutent ())
			if (ut->ut_pid == pid)
				break;

		if (ut)
			utent = *ut;

#ifdef	SVR4
		endutxent ();
#endif
		endutent ();

		if (! ut) {
 			(void) puts (NO_UTENT);
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
#endif	/* !UNIXPC */
		{
			if (! (line = ttyname (0))) {
				(void) puts (NO_TTY);
				exit (1);
			}
			if (strncmp (line, "/dev/", 5) == 0)
				line += 5;
			(void) strncpy (utent.ut_line, line,
					(int) sizeof utent.ut_line);
#ifdef	SVR4
			(void) strncpy (utxent.ut_line, line,
					(int) sizeof utxent.ut_line);
#endif
		}
	} else {
		if (! (line = ttyname (0))) {
			puts (NO_TTY);
			exit (1);
		}
		if (strncmp (line, "/dev/", 5) == 0)
			line += 5;

 		(void) strncpy (utent.ut_line, line,
  						(int) sizeof utent.ut_line);
		if (ut = getutline (&utent))
 			(void) strncpy (utent.ut_id, ut->ut_id,
 					(int) sizeof ut->ut_id);

		(void) strcpy (utent.ut_user, "LOGIN");
		utent.ut_pid = getpid ();
		utent.ut_type = LOGIN_PROCESS;
		(void) time (&utent.ut_time);
#ifdef	SVR4
		if (utx = getutxline (&utent))
			(void) strncpy (utxent.ut_id, utent.ut_id,
					(int) sizeof utxent.ut_id);

		(void) strncpy (utxent.ut_user, utent.ut_user,
			sizeof utent.ut_user);
		utxent.ut_pid = utent.ut_pid;
		utxent.ut_type = utent.ut_type;
		(void) gettimeofday ((struct timeval *) &utxent.ut_tv, 0);
		utent.ut_time = utxent.ut_tv.tv_sec;
#endif
	}
#else	/* !USG */

	/*
	 * Hand-craft a new utmp entry.
	 */

	bzero (&utent, sizeof utent);
	if (! (line = ttyname (0))) {
		puts (NO_TTY);
		exit (1);
	}
	if (strncmp (line, "/dev/", 5) == 0)
		line += 5;

	(void) strncpy (utent.ut_line, line, sizeof utent.ut_line);
	(void) time (&utent.ut_time);
#endif	/* !USG */
}

/*
 * setutmp - put a USER_PROCESS entry in the utmp file
 *
 *	setutmp changes the type of the current utmp entry to
 *	USER_PROCESS.  the wtmp file will be updated as well.
 */

void
setutmp (name, line)
char	*name;
char	*line;
{
#ifdef SVR4
	struct	utmp	*utmp, utline;
	struct	utmpx	*utmpx, utxline;
	pid_t	pid = getpid ();
	FILE	*utmpx_fp;
	int	found_utmpx = 0, found_utmp;
	int	fd;

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

	if (! found_utmpx) {
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
	if (! found_utmp) {
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

	gettimeofday (&utxline.ut_tv);
	utline.ut_time = utxline.ut_tv.tv_sec;

	strncpy (utxline.ut_host, host, sizeof utxline.ut_host);

	pututxline (&utxline);
	pututline (&utline);

	if ((fd = open (WTMP_FILE "x", O_WRONLY|O_APPEND)) != -1) {
		write (fd, (void *) &utxline, sizeof utxline);
		close (fd);
	}
	if ((fd = open (WTMP_FILE, O_WRONLY|O_APPEND)) != -1) {
		write (fd, (void *) &utline, sizeof utline);
		close (fd);
	}

	utxent = utxline;
	utent = utline;
	
#else /* !SVR4 */
	struct	utmp	utmp;
	int	fd;
	int	found = 0;

	if (! (fd = open (UTMP_FILE, O_RDWR)))
		return;

#if !defined(SUN) && !defined(BSD) && !defined(SUN4)
 	while (! found && read (fd, &utmp, sizeof utmp) == sizeof utmp) {
 		if (! strncmp (line, utmp.ut_line, (int) sizeof utmp.ut_line))
			found++;
	}
#endif
	if (! found) {

		/*
		 * This is a brand-new entry.  Clear it out and fill it in
		 * later.
		 */

  		(void) bzero (&utmp, sizeof utmp);
 		(void) strncpy (utmp.ut_line, line, (int) sizeof utmp.ut_line);
	}

	/*
	 * Fill in the parts of the UTMP entry.  BSD has just the name,
	 * while System V has the name, PID and a type.
	 */

#if defined(SUN) || defined(BSD) || defined(SUN4)
	(void) strncpy (utmp.ut_name, name, (int) sizeof utent.ut_name);
#else	/* SUN */
 	(void) strncpy (utmp.ut_user, name, (int) sizeof utent.ut_user);
	utmp.ut_type = USER_PROCESS;
	utmp.ut_pid = getpid ();
#endif	/* SUN || BSD */

	/*
	 * Put in the current time (common to everyone)
	 */

	(void) time (&utmp.ut_time);

#ifdef UT_HOST
	/*
	 * Update the host name field for systems with networking support
	 */

	(void) strncpy (utmp.ut_host, utent.ut_host, (int) sizeof utmp.ut_host);
#endif

	/*
	 * Locate the correct position in the UTMP file for this
	 * entry.
	 */

#if defined(SUN) || defined(BSD) || defined(SUN4)
	(void) lseek (fd, (long) (sizeof utmp) * ttyslot (), 0);
#else
	if (found)	/* Back up a splot */
		lseek (fd, (long) - sizeof utmp, 1);
	else		/* Otherwise, go to the end of the file */
		lseek (fd, (long) 0, 2);
#endif

	/*
	 * Scribble out the new entry and close the file.  We're done
	 * with UTMP, next we do WTMP (which is real easy, put it on
	 * the end of the file.
	 */

	(void) write (fd, &utmp, sizeof utmp);
	(void) close (fd);

	if ((fd = open (WTMP_FILE, O_WRONLY|O_APPEND)) >= 0) {
		(void) write (fd, &utmp, sizeof utmp);
		(void) close (fd);
	}
 	utent = utmp;
#endif /* SVR4 */
}
