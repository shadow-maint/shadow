/*
 * Copyright 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <fcntl.h>
#include "config.h"

#ifndef lint
static	char	sccsid[] = "@(#)mkrmdir.c	3.3	07:43:57	17 Sep 1991";
#endif

#ifdef	NEED_MKDIR
/*
 * mkdir - create a directory
 *
 *	mkdir is provided for systems which do not include the mkdir()
 *	system call.
 */

int
mkdir (dir, mode)
char	*dir;
int	mode;
{
	int	status;

	if (fork ()) {
		while (wait (&status) != -1)
			;

		return status >> 8;
	}
#ifdef	USE_SYSLOG
	closelog ();
#endif
	close (2);
	open ("/dev/null", O_WRONLY);
	umask (0777 & ~ mode);
	execl ("/bin/mkdir", "mkdir", dir, 0);
	_exit (128);
	/*NOTREACHED*/
}
#endif
#ifdef	NEED_RMDIR
/*
 * rmdir - remove a directory
 *
 *	rmdir is provided for systems which do not include the rmdir()
 *	system call.
 */

int
rmdir (dir)
char	*dir;
{
	int	status;

	if (fork ()) {
		while (wait (&status) != -1)
			;

		return status >> 8;
	}
	close (2);
	open ("/dev/null", O_WRONLY);
	execl ("/bin/rmdir", "rmdir", dir, 0);
	_exit (128);
	/*NOTREACHED*/
}
#endif
