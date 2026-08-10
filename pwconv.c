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

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include "pwd.h"
#ifndef	BSD
#include <string.h>
#else
#define	strchr	index
#define	strrchr	rindex
#include <strings.h>
#endif
#include "config.h"
#include "shadow.h"

#ifndef	lint
static	char	_sccsid[] = "@(#)pwconv.c	3.4	07:43:52	17 Sep 1991";
#endif

char	buf[BUFSIZ];

long	time ();
long	a64l ();
extern	int	getdef_num();

int	main ()
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

	if (! (pwd = fopen (PWDFILE, "r"))) {
		perror (PWDFILE);
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

	while (fgets (buf, BUFSIZ, pwd) == buf) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (buf[0] == '#') {	/* comment line */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		if (! (pw = sgetpwent (buf))) { /* copy bad lines verbatim */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		if (pw->pw_passwd[0] == '\0') { /* no password, skip */
			(void) fprintf (npwd, "%s\n", buf);
			continue;
		}
		setspent ();		/* rewind old shadow file */

		if (spwd = getspnam (pw->pw_name)) {
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
				if (strlen (pw->pw_age) >= 2) {
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
				tspwd.sp_max =
					getdef_num("PASS_MAX_DAYS", 10000);
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
