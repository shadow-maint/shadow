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

#include <pwd.h>
#include <sys/types.h>
#include <stdio.h>

#include "alloc.h"
#include "defines.h"
#include "prototypes.h"
#include "x.h"


void pw_entry (const char *name, struct passwd *pwent)
{
	struct passwd *passwd;

	struct spwd *spwd;

	if (!(passwd = getpwnam (name))) { /* local, no need for xgetpwnam */
		pwent->pw_name = NULL;
		return;
	} else {
		pwent->pw_name = x(strdup(passwd->pw_name));
		pwent->pw_uid = passwd->pw_uid;
		pwent->pw_gid = passwd->pw_gid;
		pwent->pw_gecos = x(strdup(passwd->pw_gecos));
		pwent->pw_dir = x(strdup(passwd->pw_dir));
		pwent->pw_shell = x(strdup(passwd->pw_shell));
#if !defined(AUTOSHADOW)
		/* local, no need for xgetspnam */
		if ((spwd = getspnam (name))) {
			pwent->pw_passwd = x(strdup(spwd->sp_pwdp));
			return;
		}
#endif
		pwent->pw_passwd = x(strdup(passwd->pw_passwd));
	}
}
