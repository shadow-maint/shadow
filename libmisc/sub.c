/*
 * SPDX-FileCopyrightText: 1989 - 1991, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#define	BAD_SUBROOT2	"invalid root `%s' for user `%s'\n"
#define	NO_SUBROOT2	"no subsystem root `%s' for user `%s'\n"
/*
 * subsystem - change to subsystem root
 *
 *	A subsystem login is indicated by the presence of a "*" as
 *	the first character of the login shell.  The given home
 *	directory will be used as the root of a new filesystem which
 *	the user is actually logged into.
 */
void subsystem (const struct passwd *pw)
{
	/*
	 * The new root directory must begin with a "/" character.
	 */

	if (pw->pw_dir[0] != '/') {
		printf (_("Invalid root directory '%s'\n"), pw->pw_dir);
		SYSLOG ((LOG_WARN, BAD_SUBROOT2, pw->pw_dir, pw->pw_name));
		closelog ();
		exit (EXIT_FAILURE);
	}

	/*
	 * The directory must be accessible and the current process
	 * must be able to change into it.
	 */

	if (   (chdir (pw->pw_dir) != 0)
	    || (chroot (pw->pw_dir) != 0)) {
		(void) printf (_("Can't change root directory to '%s'\n"),
		               pw->pw_dir);
		SYSLOG ((LOG_WARN, NO_SUBROOT2, pw->pw_dir, pw->pw_name));
		closelog ();
		exit (EXIT_FAILURE);
	}
}

