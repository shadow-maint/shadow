/*
 * Copyright 1990, 1991, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#ifndef	lint
static	char	sccsid[] = "@(#)mkpasswd.c	3.10	11:32:18	28 Jul 1992";
static	char	copyright[] = "Copyright 1990, 1991, 1992, John F. Haugh II";
#endif

#include "config.h"
#include <stdio.h>

#if !defined(DBM) && !defined(NDBM) /*{*/

main (argc, argv)
int	argc;
char	**argv;
{
	fprintf(stderr, "%s: no DBM database on system - no action performed\n",
		argv[0]);
	exit(0);
}

#else /*} defined(DBM) || defined(NDBM) {*/

#include <fcntl.h>
#include "pwd.h"
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#endif

#ifdef	DBM
#include <dbm.h>
#endif
#ifdef	NDBM
#include <ndbm.h>
#include <grp.h>
#include "shadow.h"

DBM	*pw_dbm;
DBM	*gr_dbm;
DBM	*sp_dbm;
DBM	*sg_dbm;
char	*fgetsx();
#endif

char	*CANT_OPEN =	"%s: cannot open file %s\n";
char	*CANT_OVERWRITE = "%s: cannot overwrite file %s\n";
#ifdef	DBM
char	*CANT_CREATE =	"%s: cannot create %s\n";
#endif
char	*DBM_OPEN_ERR =	"%s: cannot open DBM files for %s\n";
char	*PARSE_ERR =	"%s: error parsing line\n\"%s\"\n";
char	*LINE_TOO_LONG = "%s: the beginning with \"%.16s ...\" is too long\n";
char	*ADD_REC =	"adding record for name \"%s\"\n";
char	*ADD_REC_ERR =	"%s: error adding record for \"%s\"\n";
char	*INFO =		"added %d entries, longest was %d\n";
#ifdef	NDBM
char	*USAGE =	"Usage: %s [ -vf ] [ -p|g|sp|sg ] file\n";
#else
char	*USAGE =	"Usage: %s [ -vf ] file\n";
#endif

char	*Progname;
int	vflg = 0;
int	fflg = 0;
#ifdef	NDBM
int	gflg = 0;
int	sflg = 0;
int	pflg = 0;
#endif

void	usage();

extern	char	*malloc();
extern	struct	passwd	*sgetpwent();
extern	int	pw_dbm_update();
#ifdef	NDBM
extern	struct	group	*sgetgrent();
extern	struct	spwd	*sgetspent();
extern	struct	sgrp	*sgetsgent();
extern	int	sp_dbm_update();
extern	int	gr_dbm_update();
extern	int	sg_dbm_update();
#endif

/*
 * mkpasswd - create DBM files for /etc/passwd-like input file
 *
 * mkpasswd takes an an argument the name of a file in /etc/passwd format
 * and creates a DBM file keyed by user ID and name.  The output files have
 * the same name as the input file, with .dir and .pag appended.
 *
 * if NDBM is defined this command will also create look-aside files for
 * /etc/group, /etc/shadow, and /etc/gshadow.
 */

int
main (argc, argv)
int	argc;
char	**argv;
{
	extern	int	optind;
	extern	char	*optarg;
	FILE	*fp;			/* File pointer for input file        */
	char	*file;			/* Name of input file                 */
	char	*dir;			/* Name of .dir file                  */
	char	*pag;			/* Name of .pag file                  */
	char	*cp;			/* Temporary character pointer        */
	int	flag;			/* Flag for command line option       */
#ifdef	DBM
	int	fd;			/* File descriptor of open DBM file   */
#endif
	int	cnt = 0;		/* Number of entries in database      */
	int	longest = 0;		/* Longest entry in database          */
	int	len;			/* Length of input line               */
	int	errors = 0;		/* Count of errors processing file    */
	char	buf[BUFSIZ*8];		/* Input line from file               */
	struct	passwd	*passwd;	/* Pointer to password file entry     */
#ifdef	NDBM
	struct	group	*group;		/* Pointer to group file entry        */
	struct	spwd	*shadow;	/* Pointer to shadow passwd entry     */
	struct	sgrp	*gshadow;	/* Pointer to shadow group entry      */
	DBM	*dbm;			/* Pointer to new NDBM files          */
	DBM	*dbm_open();		/* Function to open NDBM files        */
#endif

	/*
	 * Figure out what my name is.  I will use this later ...
	 */

	if (Progname = strrchr (argv[0], '/'))
		Progname++;
	else
		Progname = argv[0];

	/*
	 * Figure out what the flags might be ...
	 */

#ifdef	NDBM
	while ((flag = getopt (argc, argv, "fvpgs")) != EOF)
#else
	while ((flag = getopt (argc, argv, "fv")) != EOF)
#endif
	{
		switch (flag) {
			case 'v':
				vflg++;
				break;
			case 'f':
				fflg++;
				break;
#ifdef	NDBM
			case 'g':
				gflg++;
				if (pflg)
					usage ();

				break;
			case 's':
				sflg++;
				break;
			case 'p':
				pflg++;
				if (gflg)
					usage ();

				break;
#endif
			default:
				usage ();
		}
	}

#ifdef NDBM
	/*
	 * Backwards compatibility fix for -p flag ...
	 */

	if (! sflg && ! gflg)
		pflg++;
#endif

	/*
	 * The last and only remaining argument must be the file name
	 */

	if (argc - 1 != optind)
		usage ();

	file = argv[optind];

	if (! (fp = fopen (file, "r"))) {
		fprintf (stderr, CANT_OPEN, Progname, file);
		exit (1);
	}

	/*
	 * Make the filenames for the two DBM files.
	 */

	dir = malloc (strlen (file) + 5);	/* space for .dir file */
	strcat (strcpy (dir, file), ".dir");

	pag = malloc (strlen (file) + 5);	/* space for .pag file */
	strcat (strcpy (pag, file), ".pag");

	/*
	 * Remove existing files if requested.
	 */

	if (fflg) {
		(void) unlink (dir);
		(void) unlink (pag);
	}

	/*
	 * Create the two DBM files - it is an error for these files
	 * to have existed already.
	 */

	if (access (dir, 0) == 0) {
		fprintf (stderr, CANT_OVERWRITE, Progname, dir);
		exit (1);
	}
	if (access (pag, 0) == 0) {
		fprintf (stderr, CANT_OVERWRITE, Progname, pag);
		exit (1);
	}

#ifdef	NDBM
	if (sflg)
		umask (077);
	else
#endif
	umask (0);
#ifdef	DBM
	if ((fd = open (dir, O_RDWR|O_CREAT|O_EXCL, 0644)) == -1) {
		fprintf (stderr, CANT_CREATE, Progname, dir);
		exit (1);
	} else
		close (fd);

	if ((fd = open (pag, O_RDWR|O_CREAT|O_EXCL, 0644)) == -1) {
		fprintf (stderr, CANT_CREATE, Progname, pag);
		unlink (dir);
		exit (1);
	} else
		close (fd);
#endif

	/*
	 * Now the DBM database gets initialized
	 */

#ifdef	DBM
	if (dbminit (file) == -1) {
		fprintf (stderr, DBM_OPEN_ERR, Progname, file);
		exit (1);
	}
#endif
#ifdef	NDBM
	if (! (dbm = dbm_open (file, O_RDWR|O_CREAT, 0644))) {
		fprintf (stderr, DBM_OPEN_ERR, Progname, file);
		exit (1);
	}
	if (gflg) {
		if (sflg)
			sg_dbm = dbm;
		else
			gr_dbm = dbm;
	} else {
		if (sflg)
			sp_dbm = dbm;
		else
			pw_dbm = dbm;
	}
#endif

	/*
	 * Read every line in the password file and convert it into a
	 * data structure to be put in the DBM database files.
	 */

#ifdef	NDBM
	while (fgetsx (buf, BUFSIZ, fp) != NULL)
#else
	while (fgets (buf, BUFSIZ, fp) != NULL)
#endif
	{

		/*
		 * Get the next line and strip off the trailing newline
		 * character.
		 */

		buf[sizeof buf - 1] = '\0';
		if (! (cp = strchr (buf, '\n'))) {
			fprintf (stderr, LINE_TOO_LONG, Progname, buf);
			exit (1);
		}
		*cp = '\0';
		len = strlen (buf);

		/*
		 * Parse the password file line into a (struct passwd).
		 * Erroneous lines cause error messages, but that's
		 * all.  YP lines are ignored completely.
		 */

		if (buf[0] == '-' || buf[0] == '+')
			continue;

#ifdef	DBM
		if (! (passwd = sgetpwent (buf)))
#endif
#ifdef	NDBM
		if (! (((! sflg && pflg) && (passwd = sgetpwent (buf)))
			|| ((sflg && pflg) && (shadow = sgetspent (buf)))
			|| ((! sflg && gflg) && (group = sgetgrent (buf)))
			|| ((sflg && gflg) && (gshadow = sgetsgent (buf)))))
#endif
		{
			fprintf (stderr, PARSE_ERR, Progname, buf);
			errors++;
			continue;
		}
#ifdef	DBM
		if (vflg)
			printf (ADD_REC, passwd->pw_name);

		if (! pw_dbm_update (passwd))
			fprintf (stderr, ADD_REC_ERR,
				Progname, passwd->pw_name);
#endif
#ifdef	NDBM
		if (vflg) {
			if (!sflg && pflg) printf (ADD_REC, passwd->pw_name);
			if (sflg && pflg) printf (ADD_REC, shadow->sp_namp);
			if (!sflg && gflg) printf (ADD_REC, group->gr_name);
			if (sflg && gflg) printf (ADD_REC, gshadow->sg_name);
		}
		if (! sflg && pflg && ! pw_dbm_update (passwd))
			fprintf (stderr, ADD_REC_ERR,
				Progname, passwd->pw_name);

		if (sflg && pflg && ! sp_dbm_update (shadow))
			fprintf (stderr, ADD_REC_ERR,
				Progname, shadow->sp_namp);

		if (! sflg && gflg && ! gr_dbm_update (group))
			fprintf (stderr, ADD_REC_ERR,
				Progname, group->gr_name);

		if (sflg && gflg && ! sg_dbm_update (gshadow))
			fprintf (stderr, ADD_REC_ERR,
				Progname, gshadow->sg_name);
#endif

		/*
		 * Update the longest record and record count
		 */

		if (len > longest)
			longest = len;
		cnt++;
	}

	/*
	 * Tell the user how things went ...
	 */

	if (vflg)
		printf (INFO, cnt, longest);

	exit (errors);
	/*NOTREACHED*/
}

/*
 * usage - print error message and exit
 */

void
usage ()
{
	fprintf (stderr, USAGE, Progname);
	exit (1);
	/*NOTREACHED*/
}
#endif /*} defined(DBM) || defined(NDBM) */
