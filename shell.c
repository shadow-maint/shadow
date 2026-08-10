/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 */

#include <stdio.h>
#include <errno.h>
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
static	char	_sccsid[] = "@(#)shell.c	3.2	07:55:08	06 Feb 1991";
#endif

extern	char	*newenvp[];

/*
 * shell - execute the named program
 *
 *	shell begins by trying to figure out what argv[0] is going to
 *	be for the named process.  The user may pass in that argument,
 *	or it will be the last pathname component of the file with a
 *	'-' prepended.  The first attempt is to just execute the named
 *	file.  If the errno comes back "ENOEXEC", the file is assumed
 *	at first glance to be a shell script.  The first two characters
 *	must be "#!", in which case "/bin/sh" is executed to process
 *	the file.  If all that fails, give up in disgust ...
 */

void	shell (file, arg)
char	*file;
char	*arg;
{
	char	arg0[BUFSIZ];
	FILE	*fp;
	char	*path;
	int	err;

	if (file == (char *) 0)
		exit (1);

	/*
	 * The argv[0]'th entry is usually the path name, but
	 * for various reasons the invoker may want to override
	 * that.  So, we determine the 0'th entry only if they
	 * don't want to tell us what it is themselves.
	 */

	if (arg == (char *) 0) {
		if (path = strrchr (file, '/'))
			path++;
		else
			path = file;

		(void) strcpy (arg0 + 1, path);
		arg0[0] = '-';
		arg = arg0;
	}
#ifndef	NDEBUG
	printf ("Executing shell %s\n", file);
#endif

	/*
	 * First we try the direct approach.  The system should be
	 * able to figure out what we are up to without too much
	 * grief.
	 */

	execle (file, arg, (char *) 0, newenvp);
	err = errno;

	/*
	 * It is perfectly OK to have a shell script for a login
	 * shell, and this code attempts to support that.  It
	 * relies on the standard shell being able to make sense
	 * of the "#!" magic number.
	 */

	if (err == ENOEXEC) {
		if (fp = fopen (file, "r")) {
			if (getc (fp) == '#' && getc (fp) == '!') {
				fclose (fp);
				execle ("/bin/sh", "sh",
					file, (char *) 0, newenvp);
				err = errno;
			} else {
				fclose (fp);
			}
		}
	}

	/*
	 * Obviously something is really wrong - I can't figure out
	 * how to execute this stupid shell, so I might as well give
	 * up in disgust ...
	 */

	sprintf (arg0, "Cannot execute %s", file);
	errno = err;
	perror (arg0);
	exit (err);
}
