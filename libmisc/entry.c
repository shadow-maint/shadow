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

#include "rcsid.h"
RCSID("$Id: entry.c,v 1.3 1997/12/07 23:27:03 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>

struct	passwd	*fgetpwent ();

void
entry(const char *name, struct passwd *pwent)
{
	struct	passwd	*passwd;
#ifdef	SHADOWPWD
	struct	spwd	*spwd;
#ifdef	ATT_AGE
	char	*l64a ();
	char	*cp;
#endif
#endif

	if (! (passwd = getpwnam (name))) {
		pwent->pw_name = (char *) 0;
		return;
	} else  {
		pwent->pw_name = xstrdup (passwd->pw_name);
		pwent->pw_uid = passwd->pw_uid;
		pwent->pw_gid = passwd->pw_gid;
#ifdef	ATT_COMMENT
		pwent->pw_comment = xstrdup (passwd->pw_comment);
#endif
		pwent->pw_gecos = xstrdup (passwd->pw_gecos);
		pwent->pw_dir = xstrdup (passwd->pw_dir);
		pwent->pw_shell = xstrdup (passwd->pw_shell);
#if defined(SHADOWPWD) && !defined(AUTOSHADOW)
		setspent ();
		if ((spwd = getspnam (name))) {
			pwent->pw_passwd = xstrdup (spwd->sp_pwdp);
#ifdef	ATT_AGE
			pwent->pw_age = (char *) xmalloc (5);

			if (spwd->sp_max > (63*7))
				spwd->sp_max = (63*7);
			if (spwd->sp_min > (63*7))
				spwd->sp_min = (63*7);

			pwent->pw_age[0] = i64c (spwd->sp_max / 7);
			pwent->pw_age[1] = i64c (spwd->sp_min / 7);

			cp = l64a (spwd->sp_lstchg / 7);
			pwent->pw_age[2] = cp[0];
			pwent->pw_age[3] = cp[1];

			pwent->pw_age[4] = '\0';
#endif
			endspent ();
			return;
		}
		endspent ();
#endif
		pwent->pw_passwd = xstrdup (passwd->pw_passwd);
#ifdef	ATT_AGE
		pwent->pw_age = xstrdup (passwd->pw_age);
#endif
	}
}
