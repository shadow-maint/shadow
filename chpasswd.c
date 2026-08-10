/*
 * Copyright 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * chpasswd - update passwords in batch
 *
 *	chpasswd reads standard input for a list of colon separated
 *	user names and new passwords.  the appropriate password
 *	files are updated to reflect the changes.  because the
 *	changes are made in a batch fashion, the user must run
 *	the mkpasswd command after this command terminates since
 *	no password updates occur until the very end.
 */

#include <stdio.h>
#include "pwd.h"
#include <fcntl.h>
#include <string.h>
#include "config.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)chpasswd.c	3.4	08:57:30	10 Jun 1991";
#endif

char	*Prog;

extern	char	*pw_encrypt();
extern	char	*l64a();

/* 
 * If it weren't for the different structures and differences in how
 * certain fields were manipulated, I could just use macros to replace
 * the function calls for the different file formats.  So I make the
 * best of things and just use macros to replace a few of the calls.
 */

#ifdef	SHADOWPWD
#define	pw_lock		spw_lock
#define	pw_open		spw_open
#define	pw_close	spw_close
#define	pw_unlock	spw_unlock
#endif

/*
 * usage - display usage message and exit
 */

usage ()
{
	fprintf (stderr, "usage: %s\n", Prog);
	exit (1);
}

main (argc, argv)
int	argc;
char	**argv;
{
	char	buf[BUFSIZ];
	char	*name;
	char	*newpwd;
	char	*cp;
#ifdef	SHADOWPWD
	struct	spwd	*sp;
	struct	spwd	newsp;
	struct	spwd	*spw_locate();
#else
	struct	passwd	*pw;
	struct	passwd	newpw;
	struct	passwd	*pw_locate();
	char	newage[5];
#endif
	int	errors = 0;
	int	line = 0;
	long	now = time ((long *) 0) / (24L*3600L);

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

	if (argc != 1)
		usage ();

	/*
	 * Lock the password file and open it for reading.  This will
	 * bring all of the entries into memory where they may be
	 * updated.
	 */

	if (! pw_lock ()) {
		fprintf (stderr, "%s: can't lock password file\n", Prog);
		exit (1);
	}
	if (! pw_open (O_RDWR)) {
		fprintf (stderr, "%s: can't open password file\n", Prog);
		exit (1);
	}

	/*
	 * Read each line, separating the user name from the password.
	 * The password entry for each user will be looked up in the
	 * appropriate file (shadow or passwd) and the password changed.
	 * For shadow files the last change date is set directly, for
	 * passwd files the last change date is set in the age only if
	 * aging information is present.
	 */

	while (fgets (buf, sizeof buf, stdin) != (char *) 0) {
		line++;
		if (cp = strrchr (buf, '\n')) {
			*cp = '\0';
		} else {
			fprintf (stderr, "%s: line %d: line too long\n",
				Prog, line);
			errors++;
			continue;
		}

		/*
		 * The username is the first field.  It is separated
		 * from the password with a ":" character which is
		 * replaced with a NUL to give the new password.  The
		 * new password will then be encrypted in the normal
		 * fashion with a new salt generated.
		 */

		name = buf;
		if (cp = strchr (name, ':')) {
			*cp++ = '\0';
		} else {
			fprintf (stderr, "%s: line %d: missing new password\n",
				Prog, line);
			errors++;
			continue;
		}
		newpwd = cp;
		cp = pw_encrypt (newpwd, (char *) 0);

		/*
		 * Get the password file entry for this user.  The user
		 * must already exist.
		 */

#ifdef	SHADOWPWD
		if (! (sp = spw_locate (name)))
#else
		if (! (pw = pw_locate (name)))
#endif
		{
			fprintf (stderr, "%s: line %d: unknown user %s\n",
				Prog, line, name);
			errors++;
			continue;
		}

		/*
		 * The freshly encrypted new password is merged into
		 * the user's password file entry and the last password
		 * change date is set to the current date.
		 */

#ifdef	SHADOWPWD
		newsp = *sp;
		newsp.sp_pwdp = cp;
		newsp.sp_lstchg = now;
#else
		newpw = *pw;
		newpw.pw_passwd = cp;
#ifdef	ATT_AGE
		if (newpw.pw_age[0]) {
			strcpy (newage, newpw.pw_age);
			strcpy (newage + 2, l64a (now / 7));
			newpw.pw_age = newage;
		}
#endif
#endif

		/* 
		 * The updated password file entry is then put back
		 * and will be written to the password file later, after
		 * all the other entries have been updated as well.
		 */

#ifdef	SHADOWPWD
		if (! spw_update (&newsp))
#else
		if (! pw_update (&newpw))
#endif
		{
			fprintf (stderr, "%s: line %d: cannot update password entry\n",
				Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes
	 * to be aborted.  Unlocking the password file will cause
	 * all of the changes to be ignored.  Otherwise the file is
	 * closed, causing the changes to be written out all at
	 * once, and then unlocked afterwards.
	 */

	if (errors) {
		fprintf (stderr, "%s: error detected, changes ignored\n", Prog);
		pw_unlock ();
		exit (1);
	}
	if (! pw_close ()) {
		fprintf (stderr, "%s: error updating password file\n", Prog);
		exit (1);
	}
	(void) pw_unlock ();
}
