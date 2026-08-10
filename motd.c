/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <stdio.h>
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
static	char	_sccsid[] = "@(#)motd.c	3.1	07:43:40	17 Sep 1991";
#endif

extern	char	*getdef_str();

void	motd ()
{
	FILE	*fp;
	char	motdlist[BUFSIZ], *motd, *mb;
	register int	c;

	if ((mb = getdef_str("MOTD_FILE")) == NULL)
		return;

	strncpy(motdlist, mb, sizeof(motdlist));
	motdlist[sizeof(motdlist)-1] = '\0';

	for (mb = motdlist ; (motd = strtok(mb,":")) != NULL ; mb = NULL) {
		if ((fp = fopen(motd,"r")) != NULL) {
			while ((c = getc (fp)) != EOF)
				putchar (c);
			fclose (fp);
		}
	}
	fflush (stdout);
}
