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

/*
 * Separated from setup.c.  --marekm
 */

#include <config.h>

#include "rcsid.h"
RCSID ("$Id: setugid.c,v 1.7 2003/04/22 10:59:22 kloczek Exp $")
#include <stdio.h>
#include <grp.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include "getdef.h"
/*
 * setup_uid_gid() split in two functions for PAM support -
 * pam_setcred() needs to be called after initgroups(), but
 * before setuid().
 */
int setup_groups (const struct passwd *info)
{
	/*
	 * Set the real group ID to the primary group ID in the password
	 * file.
	 */
	if (setgid (info->pw_gid) == -1) {
		perror ("setgid");
		SYSLOG ((LOG_ERR, "bad group ID `%d' for user `%s': %m\n",
			 info->pw_gid, info->pw_name));
		closelog ();
		return -1;
	}
#ifdef HAVE_INITGROUPS
	/*
	 * For systems which support multiple concurrent groups, go get
	 * the group set from the /etc/group file.
	 */
	if (initgroups (info->pw_name, info->pw_gid) == -1) {
		perror ("initgroups");
		SYSLOG ((LOG_ERR, "initgroups failed for user `%s': %m\n",
			 info->pw_name));
		closelog ();
		return -1;
	}
#endif
	return 0;
}

int change_uid (const struct passwd *info)
{
	/*
	 * Set the real UID to the UID value in the password file.
	 */
#ifndef	BSD
	if (setuid (info->pw_uid))
#else
	if (setreuid (info->pw_uid, info->pw_uid))
#endif
	{
		perror ("setuid");
		SYSLOG ((LOG_ERR, "bad user ID `%d' for user `%s': %m\n",
			 (int) info->pw_uid, info->pw_name));
		closelog ();
		return -1;
	}
	return 0;
}

/*
 *	setup_uid_gid() performs the following steps -
 *
 *	set the group ID to the value from the password file entry
 *	set the supplementary group IDs
 *	optionally call specified function which may add more groups
 *	set the user ID to the value from the password file entry
 *
 *	Returns 0 on success, or -1 on failure.
 */

int setup_uid_gid (const struct passwd *info, int is_console)
{
	if (setup_groups (info) < 0)
		return -1;

#ifdef HAVE_INITGROUPS
	if (is_console) {
		char *cp = getdef_str ("CONSOLE_GROUPS");

		if (cp && add_groups (cp))
			perror ("Warning: add_groups");
	}
#endif				/* HAVE_INITGROUPS */

	if (change_uid (info) < 0)
		return -1;

	return 0;
}
