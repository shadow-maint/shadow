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
 * 4. Neither the name of Julianne F. Haugh nor the names of its contributors
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
RCSID("$Id: chowntty.c,v 1.9 2001/06/23 11:09:02 marekm Exp $")

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <grp.h>

#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include "getdef.h"

/*
 * is_my_tty -- determine if "tty" is the same as TTY stdin is using
 */

static int
is_my_tty(const char *tty)
{
	struct	stat	by_name, by_fd;

	if (stat (tty, &by_name) || fstat (0, &by_fd))
		return 0;

	if (by_name.st_rdev != by_fd.st_rdev)
		return 0;
	else
		return 1;
}

/*
 *	chown_tty() sets the login tty to be owned by the new user ID
 *	with TTYPERM modes
 */

void
chown_tty(const char *tty, const struct passwd *info)
{
	char buf[200], full_tty[200];
	char	*group;		/* TTY group name or number */
	struct	group	*grent;
	gid_t gid;

	/*
	 * See if login.defs has some value configured for the port group
	 * ID.  Otherwise, use the user's primary group ID.
	 */

	if (! (group = getdef_str ("TTYGROUP")))
		gid = info->pw_gid;
	else if (group[0] >= '0' && group[0] <= '9')
		gid = atoi (group);
	else if ((grent = getgrnam (group)))
		gid = grent->gr_gid;
	else
		gid = info->pw_gid;

	/*
	 * Change the permissions on the TTY to be owned by the user with
	 * the group as determined above.
	 */

	if (*tty != '/') {
		snprintf(full_tty, sizeof full_tty, "/dev/%s", tty);
		tty = full_tty;
	}

	if (! is_my_tty (tty)) {
		SYSLOG((LOG_WARN, "unable to determine TTY name, got %s\n",
			tty));
		closelog();
		exit (1);
	}
	
	if (chown(tty, info->pw_uid, gid) ||
			chmod(tty, getdef_num("TTYPERM", 0600))) {
		int err = errno;

		snprintf(buf, sizeof buf, _("Unable to change tty %s"), tty);
		perror(buf);
		SYSLOG((LOG_WARN, "unable to change tty `%s' for user `%s'\n",
			tty, info->pw_name));
		closelog();

		if (!(err == EROFS && info->pw_uid == 0))
			exit(1);
	}

#ifdef __linux__
	/*
	 * Please don't add code to chown /dev/vcs* to the user logging in -
	 * it's a potential security hole.  I wouldn't like the previous user
	 * to hold the file descriptor open and watch my screen.  We don't
	 * have the *BSD revoke() system call yet, and vhangup() only works
	 * for tty devices (which vcs* is not).  --marekm
	 */
#endif
}
