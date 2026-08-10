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

#include "config.h"
#include "pwd.h"
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#ifdef	SHADOWPWD
#include "shadow.h"
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)entry.c	3.5	11:59:41	28 Dec 1991";
#endif

struct	passwd	*fgetpwent ();
char	*malloc ();

void	entry (name, pwent)
char	*name;
struct	passwd	*pwent;
{
	struct	passwd	*passwd;
#ifdef	SHADOWPWD
	struct	spwd	*spwd;
	char	*l64a ();
#endif
	char	*cp;
	char	*malloc();

	if (! (passwd = getpwnam (name))) {
		pwent->pw_name = (char *) 0;
		return;
	} else  {
		pwent->pw_name = strdup (passwd->pw_name);
		pwent->pw_uid = passwd->pw_uid;
		pwent->pw_gid = passwd->pw_gid;
#ifdef	ATT_COMMENT
		pwent->pw_comment = strdup (passwd->pw_comment);
#endif
		pwent->pw_gecos = strdup (passwd->pw_gecos);
		pwent->pw_dir = strdup (passwd->pw_dir);
		pwent->pw_shell = strdup (passwd->pw_shell);
#if defined(SHADOWPWD) && !defined(AUTOSHADOW)
		setspent ();
		if (spwd = getspnam (name)) {
			pwent->pw_passwd = strdup (spwd->sp_pwdp);
#ifdef	ATT_AGE
			pwent->pw_age = (char *) malloc (5);

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
		pwent->pw_passwd = strdup (passwd->pw_passwd);
#ifdef	ATT_AGE
		pwent->pw_age = strdup (passwd->pw_age);
#endif
	}
}
