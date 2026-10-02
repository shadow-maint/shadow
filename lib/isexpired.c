/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2001 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2009, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Extracted from age.c and made part of libshadow.a - may be useful
 * in other shadow-aware programs.  --marekm
 */

#include "config.h"

#include <sys/types.h>
#include <pwd.h>
#include <time.h>

#include "defines.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"

#ident "$Id$"


/*
 * isexpired - determine if account is expired yet
 *
 *	isexpired calculates the expiration date based on the
 *	password expiration criteria.
 *
 * Return value:
 *	0: the password is still valid
 *	1: the password has expired, it must be changed
 *	3: the account has expired
 */
int isexpired (const struct passwd *pw, /*@null@*/const struct spwd *sp)
{
	long  now;

	now = time(NULL) / DAY;

	if (NULL == sp) {
		return 0;
	}

	/*
	 * Quick and easy - there is an expired account field
	 * along with an inactive account field.  Do the expired
	 * one first since it is worse.
	 */

	if ((sp->sp_expire > 0) && (now >= sp->sp_expire)) {
		return 3;
	}

	/*
	 * Last changed date 1970-01-01 (not very likely) means that
	 * the password must be changed on next login (passwd -e).
	 *
	 * The check for "x" is a workaround for RedHat NYS libc bug -
	 * if /etc/shadow doesn't exist, getspnam() still succeeds and
	 * returns sp_lstchg==0 (must change password) instead of -1!
	 */
	if (   (0 == sp->sp_lstchg)
	    && streq(pw->pw_passwd, SHADOW_PASSWD_STRING)) {
		return 1;
	}

	return 0;
}

