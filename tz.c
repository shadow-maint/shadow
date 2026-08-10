/*
 * Copyright 1991, John F. Haugh II and Chip Rosenthal
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#ifndef lint
static	char	sccsid[] = "@(#)tz.c	3.1	07:47:56	17 Sep 1991";
#endif

#include <stdio.h>

/*
 * tz - return local timezone name
 *
 * Tz() determines the name of the local timezone by reading the
 * contents of the file named by ``fname''.
 */

char *
tz (fname)
char	*fname;
{
	FILE *fp;
	static char tzbuf[64];

	if ((fp = fopen(fname,"r")) == NULL)
		return "TZ=CST6CDT";
	else if (fgets(tzbuf, sizeof(tzbuf), fp) == NULL)
		strcpy(tzbuf, "TZ=CST6CDT");
	else
		tzbuf[strlen(tzbuf) - 1] = '\0';

	(void) fclose(fp);
	return tzbuf;
}
