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
static	char	_sccsid[] = "@(#)ttytype.c	3.3	19:40:04	28 Dec 1991";
#endif

extern	char	*getdef_str();

/*
 * ttytype - set ttytype from port to terminal type mapping database
 */

void
ttytype (line)
char	*line;
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	termvar[BUFSIZ];
	char	*typefile;
	char	*cp;
	char	*type;
	char	*port;
	char	*getenv ();

	if (getenv ("TERM"))
		return;
	if ((typefile=getdef_str("TTYTYPE_FILE")) == NULL )
		return;
	if (access (typefile, 0))
		return;

	if (! (fp = fopen (typefile, "r"))) {
		perror (typefile);
		return;
	}
	while (fgets (buf, BUFSIZ, fp)) {
		if (buf[0] == '#')
			continue;

		if (cp = strchr (buf, '\n'))
			*cp = '\0';

#if defined(SUN) || defined(BSD) || defined(SUN4)
		if ((port = strtok (buf, "\t"))
				&& (type = strtok ((char *) 0, "\t"))
				&& (type = strtok ((char *) 0, "\t"))) {
			if (strcmp (line, port) == 0)
				break;
		}	
#else	/* USG */
		if ((type = strtok (buf, " \t"))
				&& (port = strtok ((char *) 0, " \t"))) {
			if (strcmp (line, port) == 0)
				break;
		}
#endif
	}
	if (! feof (fp) && ! ferror (fp)) {
		strcat (strcpy (termvar, "TERM="), type);
		addenv (termvar);
	}
	fclose (fp);
}
