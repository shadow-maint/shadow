/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
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

#include "config.h"
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include "pwd.h"
#include "shadow.h"

#ifndef	lint
static	char	sccsid[] = "@(#)pwunconv.c	3.4	11:59:24	28 Dec 1991";
#endif

#ifdef	ITI_AGING
#define	WEEK	(7L*24L*3600L)
#else
#define	WEEK	(7)
#endif

char	buf[BUFSIZ];
char	*l64a ();

int	main ()
{
	struct	passwd	*pw;
	struct	passwd	*sgetpwent ();
	FILE	*pwd;
	FILE	*npwd;
	struct	spwd	*spwd;
	int	fd;
	char	newage[5];

	if (! (pwd = fopen (PWDFILE, "r"))) {
		perror (PWDFILE);
		return (1);
	}
	unlink ("npasswd");
	if ((fd = open ("npasswd", O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0 ||
			! (npwd = fdopen (fd, "w"))) {
		perror ("npasswd");
		return (1);
	}
	while (fgets (buf, BUFSIZ, pwd) == buf) {
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
		if (spwd->sp_max > (63*WEEK) && spwd->sp_max < 10000)
			spwd->sp_max = (63*WEEK); /* 10000 is infinity */

		if (spwd->sp_min >= 0 && spwd->sp_min <= 63*7 &&
				spwd->sp_max >= 0 && spwd->sp_max <= 63*7) {
			if (spwd->sp_lstchg == -1)
				spwd->sp_lstchg = 0;

			spwd->sp_max /= WEEK;	/* turn it into weeks */
			spwd->sp_min /= WEEK;
			spwd->sp_lstchg /= WEEK;

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
