/*
 * Copyright 1989, 1990, 1991, 1992, John F. Haugh II
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
#include <stdio.h>
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
static	char	sccsid[] = "@(#)sulog.c	3.3	13:02:53	27 Jul 1992";
#endif

extern	char	name[];
extern	char	oldname[];

time_t	time ();
extern	char	*getdef_str();

/*
 * sulog - log a SU command execution result
 */

void
sulog (tty, success)
char	*tty;		/* Name of terminal SU was executed from */
int	success;	/* Success (1) or failure (0) of command */
{
	char	*sulog;
	char	*cp;
	time_t	clock;
	struct	tm	*tm;
	struct	tm	*localtime ();
	FILE	*fp;

	if ( (sulog=getdef_str("SULOG_FILE")) == (char *) 0 )
		return;

	if ((fp = fopen (sulog, "a+")) == (FILE *) 0)
		return;			/* can't open or create logfile */

	(void) time (&clock);
	tm = localtime (&clock);

	(void) fprintf (fp, "SU %.02d/%0.2d %.02d:%.02d %c %.6s %s-%s\n",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		success ? '+':'-', tty, oldname, name);

	fflush (fp);
	fclose (fp);
}
