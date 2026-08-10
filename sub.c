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

#include <sys/types.h>
#include "config.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif

#include "pwd.h"

#ifndef	lint
static	char	sccsid[] = "@(#)sub.c	3.3	09:08:19	28 May 1991";
#endif

#define	BAD_SUBROOT	"Invalid root directory \"%s\"\n"
#define	BAD_SUBROOT2	"invalid root `%s' for user `%s'\n"
#define	NO_SUBROOT	"Can't change root directory to \"%s\"\n"
#define	NO_SUBROOT2	"no subsystem root `%s' for user `%s'\n"

/*
 * subsystem - change to subsystem root
 *
 *	A subsystem login is indicated by the presense of a "*" as
 *	the first character of the login shell.  The given home
 *	directory will be used as the root of a new filesystem which
 *	the user is actually logged into.
 */

void	subsystem (pw)
struct	passwd	*pw;
{
	/*
	 * The new root directory must begin with a "/" character.
	 */

	if (pw->pw_dir[0] != '/') {
		printf (BAD_SUBROOT, pw->pw_dir);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, BAD_SUBROOT2, pw->pw_dir, pw->pw_name);
		closelog ();
#endif
		exit (1);
	}

	/*
	 * The directory must be accessible and the current process
	 * must be able to change into it.
	 */

	if (chdir (pw->pw_dir) || chroot (pw->pw_dir)) {
		printf (NO_SUBROOT, pw->pw_dir);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, NO_SUBROOT2, pw->pw_dir, pw->pw_name);
		closelog ();
#endif
		exit (1);
	}
}
