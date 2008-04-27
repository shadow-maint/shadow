/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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
		pwent->pw_name = (char *) 0;
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
