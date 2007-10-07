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
 *
 * pwconv - convert and update shadow password files
 *
 *	Pwconv copies the old password file information to a new shadow
 *	password file, merging entries from an optional existing shadow
 *	file.
 *
 *	The new password file is left in npasswd, the new shadow file is
 *	left in nshadow.  Existing shadow entries are copied as is.
 *	New entries are created with passwords which expire in MAXDAYS days,
 *	with a last changed date of today, unless password aging
 *	information was already present.  Likewise, the minimum number of
 *	days before which the password may be changed is controlled by
 *	MINDAYS.  The number of warning days is set to WARNAGE if that
 *	macro exists.  Entries with blank passwordsare not copied to the
 *	shadow file at all.
 */

#include <config.h>
#ifndef	SHADOWPWD

main()
{
	fprintf (stderr, "Shadow passwords are not configured.\n");
	exit (1);
}

#else /*{*/

#include "rcsid.h"
RCSID("$Id: pwconv-old.c,v 1.1 1997/05/01 23:11:59 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include "defines.h"

#include "getdef.h"

static char buf[BUFSIZ];

long	a64l ();

int
main()
{
	long	today;
	struct	passwd	*pw;
	struct	passwd	*sgetpwent ();
	FILE	*pwd;
	FILE	*npwd;
	FILE	*shadow;
	struct	spwd	*spwd;
	struct	spwd	tspwd;
	int	fd;
	char	*cp;

	if (! (pwd = fopen (PASSWD_FILE, "r"))) {
		perror (PASSWD_FILE);
		exit (1);
	}
	unlink ("npasswd");
	if ((fd = open ("npasswd", O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0 ||
			! (npwd = fdopen (fd, "w"))) {
		perror ("npasswd");
		exit (1);
	}
	unlink  ("nshadow");
	if ((fd = open ("nshadow", O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0 ||
			! (shadow = fdopen (fd, "w"))) {
		perror ("nshadow");
		(void) unlink ("npasswd");
		(void) unlink ("nshadow");
		exit (1);
	}

	(void) time (&today);
	today /= (24L * 60L * 60L);

	while (fgets (buf, sizeof buf, pwd) == buf) {
		if ((cp = strrchr (buf, '\n')))
			*cp = '\0';

		if (buf[0] == '#') {	/* comment line */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		if (! (pw = sgetpwent (buf))) { /* copy bad lines verbatim */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
#if 0  /* convert all entries, even if no passwd.  --marekm */
		if (pw->pw_passwd[0] == '\0') { /* no password, skip */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
#endif
		setspent ();		/* rewind old shadow file */

#if 0
		if ((spwd = getspnam(pw->pw_name))) {
#else
		/*
		 * If the user exists, getspnam() in NYS libc (at least
		 * on Red Hat 3.0.3) always succeeds if the user exists,
		 * even if there is no /etc/shadow file.  As a result,
		 * passwords are left in /etc/passwd after pwconv!
		 *
		 * Copy existing shadow entries only if the encrypted
		 * password field in /etc/passwd is "x" - this indicates
		 * that the shadow password is really there.  --marekm
		 */
		spwd = getspnam(pw->pw_name);
		if (spwd && strcmp(pw->pw_passwd, "x") == 0) {
#endif
			if (putspent (spwd, shadow)) { /* copy old entry */
				perror ("nshadow");
				goto error;
			}
		} else {		/* need a new entry. */
			tspwd.sp_namp = pw->pw_name;
			tspwd.sp_pwdp = pw->pw_passwd;
			pw->pw_passwd = "x";
#ifdef	ATT_AGE
			if (pw->pw_age) { /* copy old password age stuff */
				if ((int) strlen (pw->pw_age) >= 2) {
					tspwd.sp_min = c64i (pw->pw_age[1]);
					tspwd.sp_max = c64i (pw->pw_age[0]);
				} else {
					tspwd.sp_min = tspwd.sp_max = -1;
				}
				if (strlen (pw->pw_age) == 4)
					tspwd.sp_lstchg = a64l (&pw->pw_age[2]);
				else
					tspwd.sp_lstchg = -1;

				/*
				 * Convert weeks to days
				 */

				if (tspwd.sp_min != -1)
					tspwd.sp_min *= 7;

				if (tspwd.sp_max != -1)
					tspwd.sp_max *= 7;

				if (tspwd.sp_lstchg != -1)
					tspwd.sp_lstchg *= 7;
			} else
#endif	/* ATT_AGE */
			{	/* fake up new password age stuff */
				tspwd.sp_max = getdef_num("PASS_MAX_DAYS", -1);
				tspwd.sp_min = getdef_num("PASS_MIN_DAYS", 0);
				tspwd.sp_lstchg = today;
			}
			tspwd.sp_warn = getdef_num("PASS_WARN_AGE", -1);
			tspwd.sp_inact = tspwd.sp_expire = tspwd.sp_flag = -1;
			if (putspent (&tspwd, shadow)) { /* output entry */
				perror ("nshadow");
				goto error;
			}
		}
		(void) fprintf (npwd, "%s:%s:%d:%d:%s:%s:",
				pw->pw_name, pw->pw_passwd,
				pw->pw_uid, pw->pw_gid,
				pw->pw_gecos, pw->pw_dir);

		if (fprintf (npwd, "%s\n",
				pw->pw_shell ? pw->pw_shell:"") == EOF) {
			perror ("npasswd");
			goto error;
		}
	}
	endspent ();

	if (ferror (npwd) || ferror (shadow)) {
		perror ("pwconv");
error:
		(void) unlink ("npasswd");
		(void) unlink ("nshadow");
		exit (1);
	}
	(void) fclose (pwd);
	(void) fclose (npwd);
	(void) fclose (shadow);

	exit (0);
}
#endif	/*}*/
