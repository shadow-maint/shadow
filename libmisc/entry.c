/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2008, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <sys/types.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>

void pw_entry (const char *name, struct passwd *pwent)
{
	struct passwd *passwd;

	struct spwd *spwd;

	if (!(passwd = getpwnam (name))) { /* local, no need for xgetpwnam */
		pwent->pw_name = NULL;
		return;
	} else {
		pwent->pw_name = xstrdup (passwd->pw_name);
		pwent->pw_uid = passwd->pw_uid;
		pwent->pw_gid = passwd->pw_gid;
		pwent->pw_gecos = xstrdup (passwd->pw_gecos);
		pwent->pw_dir = xstrdup (passwd->pw_dir);
		pwent->pw_shell = xstrdup (passwd->pw_shell);
#if !defined(AUTOSHADOW)
		/* local, no need for xgetspnam */
		if ((spwd = getspnam (name))) {
			pwent->pw_passwd = xstrdup (spwd->sp_pwdp);
			return;
		}
#endif
		pwent->pw_passwd = xstrdup (passwd->pw_passwd);
	}
}
