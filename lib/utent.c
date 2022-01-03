/*
 * SPDX-FileCopyrightText: 1993 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ifndef	HAVE_GETUTENT

#include "defines.h"
#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>

#ifndef	lint
static char rcsid[] = "$Id$";
#endif

static int utmp_fd = -1;
static struct utmp utmp_buf;

/*
 * setutent - open or rewind the utmp file
 */

void setutent (void)
{
	if (utmp_fd == -1)
		if ((utmp_fd = open (_UTMP_FILE, O_RDWR)) == -1)
			utmp_fd = open (_UTMP_FILE, O_RDONLY);

	if (utmp_fd != -1)
		lseek (utmp_fd, (off_t) 0L, SEEK_SET);
}

/*
 * endutent - close the utmp file
 */

void endutent (void)
{
	if (utmp_fd != -1)
		close (utmp_fd);

	utmp_fd = -1;
}

/*
 * getutent - get the next record from the utmp file
 */

struct utmp *getutent (void)
{
	if (utmp_fd == -1)
		setutent ();

	if (utmp_fd == -1)
		return 0;

	if (read (utmp_fd, &utmp_buf, sizeof utmp_buf) != sizeof utmp_buf)
		return 0;

	return &utmp_buf;
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif
