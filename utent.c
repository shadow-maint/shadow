/*
 * Copyright 1993, John F. Haugh II
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

#include "config.h"

#ifdef	NEED_UTENT

#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>

#ifndef	lint
static	char	sccsid[] = "@(#)utent.c	1.1	08:19:29	23 Aug 1993";
#endif

#ifndef	UTMP_FILE
#define	UTMP_FILE	"/etc/utmp"
#endif

static	int	utmp_fd = -1;
static	struct	utmp	utmp_buf;
static	struct	utmp	last_utmp_buf;

/*
 * setutent - open or rewind the utmp file
 */

void
setutent ()
{
	if (utmp_fd == -1)
		if ((utmp_fd = open (UTMP_FILE, O_RDWR)) == -1)
			utmp_fd = open (UTMP_FILE, O_RDONLY);

	if (utmp_fd != -1)
		lseek (utmp_fd, 0L, 0);
}

/*
 * endutent - close the utmp file
 */

void
endutent ()
{
	if (utmp_fd != -1)
		close (utmp_fd);

	utmp_fd = -1;
}

/*
 * getutent - get the next record from the utmp file
 */

struct utmp *
getutent ()
{
	if (utmp_fd == -1)
		setutent ();

	if (utmp_fd == -1)
		return 0;

	if (read (utmp_fd, &utmp_buf, sizeof utmp_buf) != sizeof utmp_buf)
		return 0;

	last_utmp_buf = utmp_buf;
	return &utmp_buf;
}

/*
 * getutline - get the utmp entry matching ut_line
 */

struct utmp *
getutline (utent)
#if defined(__STDC__)
const	struct	utmp	*utent;
#else
struct	utmp	*utent;
#endif
{
	struct	utmp	save;
	struct	utmp	*new;

	save = *utent;
	while (new = getutent ())
		if (strncmp (new->ut_line, save.ut_line, sizeof new->ut_line))
			continue;
		else
			return new;

	return (struct utmp *) 0;
}
#endif
