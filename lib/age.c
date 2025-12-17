/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "defines.h"
#include "exitcodes.h"
#include "prototypes.h"
#include "shadow/gshadow/endsgent.h"


#ident "$Id$"

#ifndef PASSWD_PROGRAM
#define PASSWD_PROGRAM "/bin/passwd"
#endif
/*
 * expire - force password change if password expired
 *
 *	expire() calls /bin/passwd to change the user's password
 *	if it has expired.
 */
int expire (const struct passwd *pw, /*@null@*/const struct spwd *sp)
{
	int status;
	pid_t child;
	pid_t pid;

	if (NULL == sp) {
		return 0;
	}

	/*
	 * See if the user's password has expired, and if so
	 * force them to change their password.
	 */

	status = isexpired (pw, sp);
	switch (status) {
	case 0:
		return 0;
	case 1:
		(void) fputs (_("Your password has expired."), stdout);
		break;
	case 3:
		(void) fputs (_("Your login has expired."), stdout);
		break;
	}

	/*
	 * Setting the maximum valid period to less than the minimum
	 * valid period means that the minimum period will never
	 * occur while the password is valid, so the user can never
	 * change that password.
	 */

	if (status == 3) {
		(void) puts (_("  Contact the system administrator."));
		exit (EXIT_FAILURE);
	}
	(void) puts (_("  Choose a new password."));
	(void) fflush (stdout);

	/*
	 * Close all the files so that unauthorized access won't
	 * occur.  This needs to be done anyway because those files
	 * might become stale after "passwd" is executed.
	 */

	endspent ();
	endpwent ();
#ifdef SHADOWGRP
	endsgent ();
#endif
	endgrent ();

	/*
	 * Execute the /bin/passwd command.  The exit status will be
	 * examined to see what the result is.  If there are any
	 * errors the routine will exit.  This forces the user to
	 * change their password before being able to use the account.
	 */

	pid = fork ();
	if (0 == pid) {
		int err;

		/*
		 * Set the UID to be that of the user.  This causes
		 * passwd to work just like it would had they executed
		 * it from the command line while logged in.
		 */
#if !defined(USE_PAM)
		if (setup_uid_gid (pw, false) != 0)
#else
		if (setup_uid_gid (pw) != 0)
#endif
		{
			_exit (126);
		}

		(void) execl (PASSWD_PROGRAM, PASSWD_PROGRAM, pw->pw_name, (char *) NULL);
		err = errno;
		perror ("Can't execute " PASSWD_PROGRAM);
		_exit ((ENOENT == err) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if ((pid_t) -1 == pid) {
		perror ("fork");
		exit (EXIT_FAILURE);
	}

	while (((child = wait (&status)) != pid) && (child != (pid_t)-1));

	if ((child == pid) && (0 == status)) {
		return 1;
	}

	exit (EXIT_FAILURE);
 /*@notreached@*/}
