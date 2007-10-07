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
 * pwunconv - restore old password file from shadow password file.
 *
 *	Pwunconv copies the password file information from the shadow
 *	password file, merging entries from an optional existing shadow
 *	file.
 *
 *	The new password file is left in npasswd.  There is no new
 *	shadow file.  Password aging information is translated where
 *	possible.
 */

#include <config.h>

#include "rcsid.h"
RCSID("$Id: pwunconv-old.c,v 1.1 1997/05/01 23:11:59 marekm Exp $")

#include "defines.h"
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>

#ifndef	SHADOWPWD
int
main()
{
	fprintf (stderr, "Shadow passwords are not configured.\n");
	exit (1);
}

#else	/*{*/

char	buf[BUFSIZ];
char	*l64a ();

int
main()
{
	struct	passwd	*pw;
	struct	passwd	*sgetpwent ();
	FILE	*pwd;
	FILE	*npwd;
	struct	spwd	*spwd;
	int	fd;
#ifdef	ATT_AGE
	char	newage[5];
#endif

	if (! (pwd = fopen (PASSWD_FILE, "r"))) {
		perror (PASSWD_FILE);
		return (1);
	}
	unlink ("npasswd");
	if ((fd = open ("npasswd", O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0 ||
			! (npwd = fdopen (fd, "w"))) {
		perror ("npasswd");
		return (1);
	}
	while (fgets (buf, sizeof buf, pwd) == buf) {
		buf[strlen (buf) - 1] = '\0'; /* remove '\n' character */

		if (buf[0] == '#') {	/* comment line */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		if (! (pw = sgetpwent (buf))) { /* copy bad lines verbatim */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		setspent ();		/* rewind shadow file */

		if (! (spwd = getspnam (pw->pw_name))) {
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		pw->pw_passwd = spwd->sp_pwdp;

	/*
	 * Password aging works differently in the two different systems.
	 * With shadow password files you apparently must have some aging
	 * information.  The maxweeks or minweeks may not map exactly.
	 * In pwconv we set max == 10000, which is about 30 years.  Here
	 * we have to undo that kludge.  So, if maxdays == 10000, no aging
	 * information is put into the new file.  Otherwise, the days are
	 * converted to weeks and so on.
	 */

#ifdef	ATT_AGE
		if (spwd->sp_max > (63*WEEK/SCALE) && spwd->sp_max < 10000)
			spwd->sp_max = (63*WEEK/SCALE); /* 10000 is infinity */

		if (spwd->sp_min >= 0 && spwd->sp_min <= 63*7 &&
				spwd->sp_max >= 0 && spwd->sp_max <= 63*7) {
			if (spwd->sp_lstchg == -1)
				spwd->sp_lstchg = 0;

			spwd->sp_max /= WEEK/SCALE;	/* turn it into weeks */
			spwd->sp_min /= WEEK/SCALE;
			spwd->sp_lstchg /= WEEK/SCALE;

			strncpy (newage, l64a (spwd->sp_lstchg * (64L*64L) +
				  spwd->sp_min * (64L) + spwd->sp_max), 5);
			pw->pw_age = newage;
		} else
			pw->pw_age = "";
#endif	/* ATT_AGE */
		if (putpwent (pw, npwd)) {
			perror ("pwunconv: write error");
			exit (1);
		}
	}
	endspent ();

	if (ferror (npwd)) {
		perror ("pwunconv");
		(void) unlink ("npasswd");
	}
	(void) fclose (npwd);
	(void) fclose (pwd);
	return (0);
}
#endif
