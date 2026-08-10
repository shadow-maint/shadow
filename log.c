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

#include <sys/types.h>
#include <utmp.h>
#ifdef	SVR4
#include <utmpx.h>
#endif
#include "pwd.h"
#include <fcntl.h>
#include <time.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include "config.h"

#ifndef	lint
static	char	sccsid[] = "@(#)log.c	3.7	08:07:11	19 Jul 1993";
#endif

#include "lastlog.h"

#ifndef	LASTLOG_FILE
#ifdef	SVR4
#define	LASTLOG_FILE	"/var/adm/lastlog"
#else
#define	LASTLOG_FILE	"/usr/adm/lastlog"
#endif	/* SVR4 */
#endif	/* LASTLOG_FILE */

extern	struct	utmp	utent;
#ifdef	SVR4
extern	struct	utmpx	utxent;
#endif
extern	struct	passwd	pwent;
extern	struct	lastlog	lastlog;
extern	char	**environ;

long	lseek ();
time_t	time ();

/* 
 * log - create lastlog entry
 *
 *	A "last login" entry is created for the user being logged in.  The
 *	UID is extracted from the global (struct passwd) entry and the
 *	TTY information is gotten from the (struct utmp).
 */

void	log ()
{
	int	fd;
	off_t	offset;
	struct	lastlog	newlog;

	/*
	 * If the file does not exist, don't create it.
	 */

	if ((fd = open (LASTLOG_FILE, O_RDWR)) == -1)
		return;

	/*
	 * The file is indexed by UID number.  Seek to the record
	 * for this UID.  Negaive UID's will create problems, but ...
	 */

	offset = pwent.pw_uid * sizeof lastlog;

	if (lseek (fd, offset, 0) != offset) {
		(void) close (fd);
		return;
	}

	/*
	 * Read the old entry so we can tell the user when they last
	 * logged in.  Then construct the new entry and write it out
	 * the way we read the old one in.
	 */

	if (read (fd, (char *) &lastlog, sizeof lastlog) != sizeof lastlog)
#ifndef	BSD
		memset ((char *) &lastlog, sizeof lastlog, 0);
#else
		bzero ((char *) &lastlog, sizeof lastlog);
#endif
	newlog = lastlog;

	(void) time (&newlog.ll_time);
	(void) strncpy (newlog.ll_line, utent.ut_line, sizeof newlog.ll_line);
#ifdef	SVR4
	(void) strncpy (newlog.ll_host, utxent.ut_host, sizeof newlog.ll_host);
#endif
	(void) lseek (fd, offset, 0);
	(void) write (fd, (char *) &newlog, sizeof newlog);
	(void) close (fd);
}
