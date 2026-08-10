/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "config.h"
#include "pwd.h"
#include "shadow.h"

#ifndef	lint
static	char	sccsid[] = "@(#)age.c	3.7	21:46:45	02 Jun 1993";
#endif

static	char	*EXPIRE_DAY = "Your password will expire in 1 day.\n";
static	char	*EXPIRE_DAYS = "Your password will expired in %d days.\n";
static	char	*PASSWORD_EXPIRED = "Your password has expired.";
static	char	*PASSWORD_INACTIVE = "Your password is inactive.";
static	char	*LOGIN_EXPIRED = "Your login has expired.";
static	char	*CONTACT_SYSADM = "  Contact the system administrator.\n";
static	char	*NEW_PASSWORD = "  Choose a new password.\n";

#define	DAY	(24L*3600L)
#ifdef	ITI_AGING
#define	SCALE	(DAY)
#else
#define	SCALE	(1)
#endif

extern	time_t	time ();
extern	char	*strdup();

/*
 * pwd_to_spwd - create entries for new spwd structure
 *
 *	pwd_to_spwd() creates a new (struct spwd) containing the
 *	information in the pointed-to (struct passwd).
 */

static struct spwd *
pwd_to_spwd (pw)
struct	passwd	*pw;
{
	static	struct	spwd	tspwd;
	struct	spwd	*sp = &tspwd;
	time_t	t;

	/*
	 * Nice, easy parts first.  The name and passwd map directly
	 * from the old password structure to the new one.
	 */

	sp->sp_namp = strdup (pw->pw_name);
	sp->sp_pwdp = strdup (pw->pw_passwd);
#ifdef	ATT_AGE

	/*
	 * AT&T-style password aging maps the sp_min, sp_max, and
	 * sp_lstchg information from the pw_age field, which appears
	 * after the encrypted password.
	 */

	if (pw->pw_age[0]) {
		t = (c64i (pw->pw_age[0]) * 7) * SCALE;
		sp->sp_max = t;

		if (pw->pw_age[1]) {
			t = (c64i (pw->pw_age[1]) * 7) * SCALE;
			sp->sp_min = t;
		} else
			sp->sp_min = (10000L) * SCALE;

		if (pw->pw_age[1] && pw->pw_age[2]) {
			t = (a64l (pw->pw_age + 2) * 7) * SCALE;
			sp->sp_lstchg = t;
		} else
			sp->sp_lstchg = time ((time_t *) 0) / (DAY/SCALE);
	} else {
		sp->sp_min = 0;
		sp->sp_max = (10000L * SCALE);
		sp->sp_lstchg = time ((time_t *) 0) / (DAY/SCALE);
	}
#else	/* !ATT_AGE */
	/*
	 * BSD does not use the pw_age field and has no aging information
	 * anywheres.  The default values are used to initialize the
	 * fields which are in the missing pw_age field;
	 */

	sp->sp_min = 0;
	sp->sp_max = (10000L * SCALE);
	sp->sp_lstchg = time ((time_t *) 0) / (DAY/SCALE);
#endif	/* ATT_AGE */

	/*
	 * These fields have no corresponding information in the password
	 * file.  They are set to uninitialized values.
	 */

	sp->sp_warn = -1;
	sp->sp_inact = -1;
	sp->sp_expire = -1;
	sp->sp_flag = -1;

	return sp;
}

/*
 * isexpired - determine if account is expired yet
 *
 *	isexpired calculates the expiration date based on the
 *	password expiration criteria.
 */

/*ARGSUSED*/
int
isexpired (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
{
	long	clock;

	clock = time ((time_t *) 0) / (DAY/SCALE);

	/*
	 * Quick and easy - there is an expired account field
	 * along with an inactive account field.  Do the expired
	 * one first since it is worse.
	 */

	if (sp->sp_expire > 0 && sp->sp_expire < clock)
		return 3;

	if (sp->sp_inact > 0 && sp->sp_lstchg > 0 && sp->sp_max > 0 &&
			sp->sp_inact + sp->sp_lstchg + sp->sp_max < clock)
		return 2;

	/*
	 * The last and max fields must be present for an account
	 * to have an expired password.  A maximum of >10000 days
	 * is considered to be infinite.
	 */

	if (sp->sp_lstchg == -1 ||
			sp->sp_max == -1 || sp->sp_max >= (10000L*SCALE))
		return 0;

	/*
	 * Calculate today's day and the day on which the password
	 * is going to expire.  If that date has already passed,
	 * the password has expired.
	 */

	if (sp->sp_lstchg + sp->sp_max < clock)
		return 1;

	return 0;
}

/*
 * expire - force password change if password expired
 *
 *	expire() calls /bin/passwd to change the user's password
 *	if it has expired.
 */

int
expire (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
{
	int	status;
	int	child;
	int	pid;

	if (! sp)
		sp = pwd_to_spwd (pw);

	/*
	 * See if the user's password has expired, and if so
	 * force them to change their password.
	 */

	switch (status = isexpired (pw, sp)) {
		case 0:
			return 0;
		case 1:
			printf (PASSWORD_EXPIRED);
			break;
		case 2:
			printf (PASSWORD_INACTIVE);
			break;
		case 3:
			printf (LOGIN_EXPIRED);
			break;
	}

	/*
	 * Setting the maximum valid period to less than the minimum
	 * valid period means that the minimum period will never
	 * occur while the password is valid, so the user can never
	 * change that password.
	 */

	if (status > 1 || sp->sp_max < sp->sp_min) {
		puts (CONTACT_SYSADM);
		exit (1);
	}
	puts (NEW_PASSWORD);
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

		/*
		 * Set the UID to be that of the user.  This causes
		 * passwd to work just like it would had they executed
		 * it from the command line while logged in.
		 */

		if (setuid (pw->pw_uid))
			_exit (errno);

		execl ("/bin/passwd", "passwd", pw->pw_name, (char *) 0);
		puts ("Can't execute /bin/passwd");
		fflush (stdout);

		_exit (errno);
	} else if (pid == -1) {
		perror ("passwd");
		exit (errno);
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

void
agecheck (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
{
	long	clock = time ((long *) 0) / (DAY/SCALE);
	long	remain;

	if (! sp)
		sp = pwd_to_spwd (pw);

	/*
	 * The last, max, and warn fields must be supported or the
	 * warning period cannot be calculated.
	 */

	if (sp->sp_lstchg == -1 || sp->sp_max == -1 || sp->sp_warn == -1)
		return;

	if ((remain = (sp->sp_lstchg + sp->sp_max) - clock) <= sp->sp_warn) {
		remain /= SCALE;
		if (remain >= 0) {
			if (remain == 1)
				printf (EXPIRE_DAY);
			else
				printf (EXPIRE_DAYS, remain);
		}
	}
}
