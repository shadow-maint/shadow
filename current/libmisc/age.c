/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <grp.h>
#ifdef HAVE_USERSEC_H
#include <userpw.h>
#include <usersec.h>
#include <userconf.h>
#endif

#ifndef	AGING
#if defined(SHADOWPWD) || defined(HAVE_USERSEC_H)
#define AGING	1
#endif
#else
#if !defined(SHADOWPWD) && !defined(HAVE_USERSEC_H) && !defined(ATT_AGE)
#undef AGING
#endif
#endif

#if defined(SHADOWPWD) || defined(AGING) /*{*/

#include "rcsid.h"
RCSID("$Id: age.c,v 1.6 1998/12/28 20:34:42 marekm Exp $")

#ifndef PASSWD_PROGRAM
#define PASSWD_PROGRAM "/bin/passwd"
#endif

/*
 * expire - force password change if password expired
 *
 *	expire() calls /bin/passwd to change the user's password
 *	if it has expired.
 */

#ifdef	SHADOWPWD
int
expire(const struct passwd *pw, const struct spwd *sp)
{
#else
int
expire(const struct passwd *pw)
{
#endif
	int	status;
	int	child;
	int	pid;

#ifdef	SHADOWPWD
	if (! sp)
		sp = pwd_to_spwd (pw);
#endif

	/*
	 * See if the user's password has expired, and if so
	 * force them to change their password.
	 */

#ifdef	SHADOWPWD
	switch (status = isexpired (pw, sp))
#else
	switch (status = isexpired (pw))
#endif
	{
		case 0:
			return 0;
		case 1:
			printf(_("Your password has expired."));
			break;
		case 2:
			printf(_("Your password is inactive."));
			break;
		case 3:
			printf(_("Your login has expired."));
			break;
	}

	/*
	 * Setting the maximum valid period to less than the minimum
	 * valid period means that the minimum period will never
	 * occur while the password is valid, so the user can never
	 * change that password.
	 */

#ifdef	SHADOWPWD
	if (status > 1 || sp->sp_max < sp->sp_min)
#else
	if (status > 1 || c64i (pw->pw_age[0]) < c64i (pw->pw_age[1]))
#endif
	{
		puts(_("  Contact the system administrator.\n"));
		exit(1);
	}
	puts(_("  Choose a new password.\n"));
	fflush (stdout);

	/*
	 * Close all the files so that unauthorized access won't
	 * occur.  This needs to be done anyway because those files
	 * might become stale after "passwd" is executed.
	 */

#ifdef	SHADOWPWD
	endspent ();
#endif
	endpwent ();
#ifdef	SHADOWGRP
	endsgent ();
#endif
	endgrent ();

	/*
	 * Execute the /bin/passwd command.  The exit status will be
	 * examined to see what the result is.  If there are any
	 * errors the routine will exit.  This forces the user to
	 * change their password before being able to use the account.
	 */

	if ((pid = fork ()) == 0) {
		int err;

		/*
		 * Set the UID to be that of the user.  This causes
		 * passwd to work just like it would had they executed
		 * it from the command line while logged in.
		 */
		if (setup_uid_gid(pw, 0))
			_exit(126);

		execl(PASSWD_PROGRAM, PASSWD_PROGRAM, pw->pw_name, (char *)0);
		err = errno;
		perror("Can't execute " PASSWD_PROGRAM);
		_exit((err == ENOENT) ? 127 : 126);
	} else if (pid == -1) {
		perror("fork");
		exit(1);
	}
	while ((child = wait (&status)) != pid && child != -1)
		;

	if (child == pid && status == 0)
		return 1;

	exit (1);
	/*NOTREACHED*/
}

/*
 * agecheck - see if warning is needed for password expiration
 *
 *	agecheck sees how many days until the user's password is going
 *	to expire and warns the user of the pending password expiration.
 */

#ifdef	SHADOWPWD
void
agecheck(const struct passwd *pw, const struct spwd *sp)
{
#else
void
agecheck(const struct passwd *pw)
{
#endif
	long	now = time ((long *) 0) / SCALE;
	long	remain;

#ifdef	SHADOWPWD
	if (! sp)
		sp = pwd_to_spwd (pw);

	/*
	 * The last, max, and warn fields must be supported or the
	 * warning period cannot be calculated.
	 */

	if (sp->sp_lstchg == -1 || sp->sp_max == -1 || sp->sp_warn == -1)
		return;
#else
	if (pw->pw_age[0] == '\0')
		return;
#endif

#ifdef	SHADOWPWD
	if ((remain = (sp->sp_lstchg + sp->sp_max) - now) <= sp->sp_warn)
#else
	if ((remain = (a64l (pw->pw_age + 2) + c64i (pw->pw_age[0])) * 7
			- now) <= getdef_num ("PASS_WARN_AGE", 7))
#endif
	{
		remain /= DAY/SCALE;
		if (remain > 1)
			printf(_("Your password will expire in %ld days.\n"), remain);
		else if (remain == 1)
			printf(_("Your password will expire tomorrow.\n"));
		else if (remain == 0)
			printf(_("Your password will expire today.\n"));
	}
}
#endif /*}*/
