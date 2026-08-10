/*
 * Copyright 1990, 1991, 1992, 1993 John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif
#include "config.h"
#include "dialup.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)dpmain.c	3.9	08:07:07	19 Jul 1993";
#endif

#ifdef	USG
#define	bzero(p,l)	memset(p, 0, l)
#endif

#define	DTMP	"/etc/d_passwd.tmp"

/*
 * Prompts and messages go here.
 */

char	*PASS1 = "Shell password:";
char	*PASS2 = "re-enter Shell password:";
char	*NOMATCH = "%s: Passwords do not match, try again.\n";
char	*NOFOUND = "%s: Shell %s not found.\n";

#define	DIALCHG	"changed password for %s\n"
#define	DIALADD "added password for %s\n"
#define	DIALREM "removed password for %s\n"

int	aflg;
int	dflg;
char	*Prog;

extern	char	*pw_encrypt();
extern	char	*getpass();

usage ()
{
	fprintf (stderr, "Usage: %s [ -(a|d) ] shell\n", Prog);
	exit (1);
}

main (argc, argv)
int	argc;
char	**argv;
{
	struct	dialup	*dial;
	struct	dialup	dent;
	struct	stat	sb;
	FILE	*fp;
	char	*shell = 0;
	char	*cp;
	char	pass[BUFSIZ];
	int	fd;
	int	found = 0;
	int	opt;
	extern	int	optind;
	extern	char	*optarg;

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif

	while ((opt = getopt (argc, argv, "a:d:")) != EOF) {
		switch (opt) {
			case 'a':
				aflg++;
				shell = optarg;
				break;
			case 'd':
				dflg++;
				shell = optarg;
				break;
			default:
				usage ();
		}
	}
	if (! aflg && ! dflg)
		aflg++;

	if (! shell) {
		if (optind >= argc)
			usage ();
		else
			shell = argv[optind];
	}
	if (aflg + dflg != 1)
		usage ();

	/*
	 * Add a new shell to the password file, or update an existing
	 * entry.  Begin by getting an encrypted password for this
	 * shell.
	 */

	if (aflg) {
		int	tries = 3;

		dent.du_shell = shell;
		dent.du_passwd = "";

again:
		if (! (cp = getpass (PASS1)))
			exit (1);

		strcpy (pass, cp);
		bzero (cp, strlen (cp));

		if (! (cp = getpass (PASS2)))
			exit (1);

		if (strcmp (pass, cp)) {
			bzero (pass, strlen (pass));
			bzero (cp, strlen (cp));
			fprintf (stderr, NOMATCH, Prog);

			if (--tries)
				goto again;

			exit (1);
		}
		bzero (cp, strlen (cp));
		dent.du_passwd = pw_encrypt (pass, (char *) 0);
		bzero (pass, strlen (pass));
	}

	/*
	 * Create the temporary file for the updated dialup password
	 * information to be placed into.  Turn it into a (FILE *)
	 * for use by putduent().
	 */

	if ((fd = open (DTMP, O_CREAT|O_EXCL|O_RDWR, 0600)) < 0) {
		sprintf (pass, "%s: can't create %s", Prog, DTMP);
		perror (pass);
		exit (1);
	}
	if (! (fp = fdopen (fd, "r+"))) {
		sprintf (pass, "%s: can't open %s", Prog, DTMP);
		perror (pass);
		unlink (DTMP);
		exit (1);
	}

	/*
	 * Scan the dialup password file for the named entry,
	 * copying out other entries along the way.  Copying
	 * stops when a match is found or the file runs out.
	 */

	while (dial = getduent ()) {
		if (strcmp (dial->du_shell, shell) == 0) {
			found = 1;
			break;
		}
		if (putduent (dial, fp))
			goto failure;
	}

	/*
	 * To delete the entry, just don't copy it.  To update
	 * the entry, output the modified version - works with
	 * new entries as well.
	 */

	if (dflg && ! found) {
		fprintf (stderr, NOFOUND, Prog, shell);
		goto failure;
	}
	if (aflg)
		if (putduent (&dent, fp))
			goto failure;

	/*
	 * Now copy out the remaining entries.  Flush and close the
	 * new file before doing anything nasty to the existing
	 * file.
	 */


	while (dial = getduent ())
		if (putduent (dial, fp))
			goto failure;

	if (fflush (fp))
		goto failure;

	fclose (fp);

	/*
	 * If the original file did not exist, we must create a new
	 * file with owner "root" and mode 400.  Otherwise we copy
	 * the modes from the existing file to the new file.
	 *
	 * After this is done the new file will replace the old file.
	 */

	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif
	if (! stat (DIALPWD, &sb)) {
		chown (DTMP, sb.st_uid, sb.st_gid);
		chmod (DTMP, sb.st_mode);
		unlink (DIALPWD);
	} else {
		chown (DTMP, 0, 0);
		chmod (DTMP, 0400);
	}
	if (! link (DTMP, DIALPWD))
		unlink (DTMP);

#ifdef	USE_SYSLOG
	if (aflg && ! found)
		syslog (LOG_INFO, DIALADD, shell);
	else if (aflg && found)
		syslog (LOG_INFO, DIALCHG, shell);
	else if (dflg)
		syslog (LOG_INFO, DIALREM, shell);

	closelog ();
#endif
	sync ();
	exit (0);

failure:
	unlink (DTMP);
#ifdef	USE_SYSLOG
	closelog ();
#endif
	exit (1);
}
