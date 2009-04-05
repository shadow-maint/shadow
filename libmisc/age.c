/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1998, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2008       , Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "prototypes.h"
#include "defines.h"
#include "exitcodes.h"
#include <pwd.h>
#include <grp.h>

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
int expire (const struct passwd *pw, const struct spwd *sp)
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
	case 2:
		(void) fputs (_("Your password is inactive."), stdout);
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

	if ((status > 1) || (sp->sp_max < sp->sp_min)) {
		(void) puts (_("  Contact the system administrator."));
		exit (1);
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
#if defined(HAVE_INITGROUPS) && ! defined(USE_PAM)
		if (setup_uid_gid (pw, false) != 0)
#else
		if (setup_uid_gid (pw) != 0)
#endif
		{
			_exit (126);
		}

		execl (PASSWD_PROGRAM, PASSWD_PROGRAM, pw->pw_name, (char *) 0);
		err = errno;
		perror ("Can't execute " PASSWD_PROGRAM);
		_exit ((ENOENT == err) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if ((pid_t) -1 == pid) {
		perror ("fork");
		exit (1);
	}

	while (((child = wait (&status)) != pid) && (child != (pid_t)-1));

	if ((child == pid) && (0 == status)) {
		return 1;
	}

	exit (1);
 /*NOTREACHED*/}

/*
 * agecheck - see if warning is needed for password expiration
 *
 *	agecheck sees how many days until the user's password is going
 *	to expire and warns the user of the pending password expiration.
 */

void agecheck (const struct passwd *pw, const struct spwd *sp)
{
	long now = (long) time ((time_t *) 0) / SCALE;
	long remain;

	if (NULL == sp) {
		return;
	}

	/*
	 * The last, max, and warn fields must be supported or the
	 * warning period cannot be calculated.
	 */

	if (   (-1 == sp->sp_lstchg)
	    || (-1 == sp->sp_max)
	    || (-1 == sp->sp_warn)) {
		return;
	}

	if (0 == sp->sp_lstchg) {
		(void) puts (_("You must change your password."));
		return;
	}

	remain = sp->sp_lstchg + sp->sp_max - now;
	if (remain <= sp->sp_warn) {
		remain /= DAY / SCALE;
		if (remain > 1) {
			(void) printf (_("Your password will expire in %ld days.\n"),
			               remain);
		} else if (1 == remain) {
			(void) puts (_("Your password will expire tomorrow."));
		} else if (remain == 0) {
			(void) puts (_("Your password will expire today."));
		}
	}
}

