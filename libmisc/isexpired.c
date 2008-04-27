/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
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

/*
 * Extracted from age.c and made part of libshadow.a - may be useful
 * in other shadow-aware programs.  --marekm
 */

#include <config.h>

#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <time.h>

#ident "$Id$"


/*
 * isexpired - determine if account is expired yet
 *
 *	isexpired calculates the expiration date based on the
 *	password expiration criteria.
 */
 /*ARGSUSED*/ int isexpired (const struct passwd *pw, const struct spwd *sp)
{
	long now;

	now = time ((time_t *) 0) / SCALE;

	if (!sp)
		sp = pwd_to_spwd (pw);

	/*
	 * Quick and easy - there is an expired account field
	 * along with an inactive account field.  Do the expired
	 * one first since it is worse.
	 */

	if (sp->sp_expire > 0 && now >= sp->sp_expire)
		return 3;

	/*
	 * Last changed date 1970-01-01 (not very likely) means that
	 * the password must be changed on next login (passwd -e).
	 *
	 * The check for "x" is a workaround for RedHat NYS libc bug -
	 * if /etc/shadow doesn't exist, getspnam() still succeeds and
	 * returns sp_lstchg==0 (must change password) instead of -1!
	 */
	if (sp->sp_lstchg == 0 && !strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING))
		return 1;

	if (sp->sp_lstchg > 0 && sp->sp_max >= 0 && sp->sp_inact >= 0 &&
	    now >= sp->sp_lstchg + sp->sp_max + sp->sp_inact)
		return 2;

	/*
	 * The last and max fields must be present for an account
	 * to have an expired password.  A maximum of >10000 days
	 * is considered to be infinite.
	 */

	if (sp->sp_lstchg == -1 ||
	    sp->sp_max == -1 || sp->sp_max >= (10000L * DAY / SCALE))
		return 0;

	/*
	 * Calculate today's day and the day on which the password
	 * is going to expire.  If that date has already passed,
	 * the password has expired.
	 */

	if (now >= sp->sp_lstchg + sp->sp_max)
		return 1;
	return 0;
}
