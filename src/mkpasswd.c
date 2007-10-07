/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: mkpasswd.c,v 1.6 1999/06/07 16:40:45 marekm Exp $")

#include <sys/stat.h>
#include "prototypes.h"
#include "defines.h"
#include <stdio.h>

#if !defined(NDBM) /*{*/
int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fprintf(stderr,
		_("%s: no DBM database on system - no action performed\n"),
		argv[0]);
	return 0;
}

#else /*} defined(NDBM) {*/

#include <fcntl.h>
#include <pwd.h>

#include <ndbm.h>
#include <grp.h>

extern	DBM	*pw_dbm;
extern	DBM	*gr_dbm;
#ifdef	SHADOWPWD
extern	DBM	*sp_dbm;
#endif
#ifdef	SHADOWGRP
extern	DBM	*sg_dbm;
#endif
char	*fgetsx();

#ifdef	SHADOWPWD
#ifdef	SHADOWGRP
#define USAGE		_("Usage: %s [ -vf ] [ -p|g|sp|sg ] file\n")
#else	/* !SHADOWGRP */
#define USAGE		_("Usage: %s [ -vf ] [ -p|g|sp ] file\n")
#endif	/* SHADOWGRP */
#else	/* !SHADOWPWD */
#define USAGE		_("Usage: %s [ -vf ] [ -p|g ] file\n")
#endif	/* SHADOWPWD */

char	*Progname;
int	vflg = 0;
int	fflg = 0;
int	gflg = 0;
int	sflg = 0;		/* -s flag -- leave in, makes code nicer */
int	pflg = 0;

extern	struct	passwd	*sgetpwent();
extern	int	pw_dbm_update();

extern	struct	group	*sgetgrent();
extern	int	gr_dbm_update();

#ifdef	SHADOWPWD
extern	struct	spwd	*sgetspent();
extern	int	sp_dbm_update();
#endif

#ifdef	SHADOWGRP
extern	struct	sgrp	*sgetsgent();
extern	int	sg_dbm_update();
#endif

/* local function prototypes */
int main P_((int, char **));
static void usage P_((void));

/*
 * mkpasswd - create DBM files for /etc/passwd-like input file
 *
 * mkpasswd takes an an argument the name of a file in /etc/passwd format
 * and creates a DBM file keyed by user ID and name.  The output files have
 * the same name as the input file, with .dir and .pag appended.
 *
 * this command will also create look-aside files for
 * /etc/group, /etc/shadow, and /etc/gshadow.
 */

int
main(int argc, char **argv)
{
	extern	int	optind;
	extern	char	*optarg;
	FILE	*fp;			/* File pointer for input file       */
	char	*file;			/* Name of input file                */
	char	*dir;			/* Name of .dir file                 */
	char	*pag;			/* Name of .pag file                 */
	char	*cp;			/* Temporary character pointer       */
	int	flag;			/* Flag for command line option      */
	int	cnt = 0;		/* Number of entries in database     */
	int	longest = 0;		/* Longest entry in database         */
	int	len;			/* Length of input line              */
	int	errors = 0;		/* Count of errors processing file   */
	char	buf[BUFSIZ*8];		/* Input line from file              */
	struct	passwd	*passwd=NULL;	/* Pointer to password file entry    */

	struct	group	*group=NULL;   	/* Pointer to group file entry       */
#ifdef	SHADOWPWD
	struct	spwd	*shadow=NULL;	/* Pointer to shadow passwd entry    */
#endif
#ifdef	SHADOWGRP
	struct	sgrp	*gshadow=NULL;	/* Pointer to shadow group entry     */
#endif
	DBM	*dbm;			/* Pointer to new NDBM files         */
	DBM	*dbm_open();		/* Function to open NDBM files       */

	/*
	 * Figure out what my name is.  I will use this later ...
	 */

	Progname = Basename(argv[0]);

	/*
	 * Figure out what the flags might be ...
	 */

	while ((flag = getopt (argc, argv, "fvpgs")) != EOF) {
		switch (flag) {
			case 'v':
				vflg++;
				break;
			case 'f':
				fflg++;
				break;
			case 'g':
				gflg++;
#ifndef	SHADOWGRP
				if (sflg)
					usage ();
#endif
				if (pflg)
					usage ();

				break;
#if defined(SHADOWPWD) || defined(SHADOWGRP)
			case 's':
				sflg++;
#ifndef	SHADOWGRP
				if (gflg)
					usage ();
#endif
				break;
#endif
			case 'p':
				pflg++;
				if (gflg)
					usage ();

				break;
			default:
				usage();
		}
	}

	/*
	 * Backwards compatibility fix for -p flag ...
	 */

#ifdef SHADOWPWD
	if (! sflg && ! gflg)
#else
	if (! gflg)
#endif
		pflg++;

	/*
	 * The last and only remaining argument must be the file name
	 */

	if (argc - 1 != optind)
		usage ();

	file = argv[optind];

	if (! (fp = fopen (file, "r"))) {
		fprintf (stderr, _("%s: cannot open file %s\n"), Progname, file);
		exit (1);
	}

	/*
	 * Make the filenames for the two DBM files.
	 */

	dir = xmalloc (strlen (file) + 5);	/* space for .dir file */
	strcat (strcpy (dir, file), ".dir");

	pag = xmalloc (strlen (file) + 5);	/* space for .pag file */
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

	if (access(dir, F_OK) == 0) {
		fprintf (stderr, _("%s: cannot overwrite file %s\n"), Progname, dir);
		exit (1);
	}
	if (access(pag, F_OK) == 0) {
		fprintf (stderr, _("%s: cannot overwrite file %s\n"), Progname, pag);
		exit (1);
	}

	if (sflg)
		umask(077);
	else
		umask(022);

	/*
	 * Now the DBM database gets initialized
	 */

	if (! (dbm = dbm_open (file, O_RDWR|O_CREAT, 0644))) {
		fprintf (stderr, _("%s: cannot open DBM files for %s\n"), Progname, file);
		exit (1);
	}
	if (gflg) {
#ifdef	SHADOWGRP
		if (sflg)
			sg_dbm = dbm;
		else
#endif
			gr_dbm = dbm;
	} else {
#ifdef	SHADOWPWD
		if (sflg)
			sp_dbm = dbm;
		else
#endif
			pw_dbm = dbm;
	}

	/*
	 * Read every line in the password file and convert it into a
	 * data structure to be put in the DBM database files.
	 */

	while (fgetsx (buf, BUFSIZ, fp) != NULL) {

		/*
		 * Get the next line and strip off the trailing newline
		 * character.
		 */

		buf[sizeof buf - 1] = '\0';
		if (! (cp = strchr (buf, '\n'))) {
			fprintf (stderr, _("%s: the beginning with "%.16s ..." is too long\n"), Progname, buf);
			exit (1);
		}
		*cp = '\0';
		len = strlen (buf);

#ifdef	USE_NIS
		/*
		 * Parse the password file line into a (struct passwd).
		 * Erroneous lines cause error messages, but that's
		 * all.  YP lines are ignored completely.
		 */

		if (buf[0] == '-' || buf[0] == '+')
			continue;
#endif
		if (! (((! sflg && pflg) && (passwd = sgetpwent (buf)))
#ifdef	SHADOWPWD
			|| ((sflg && pflg) && (shadow = sgetspent (buf)))
#endif
			|| ((! sflg && gflg) && (group = sgetgrent (buf)))
#ifdef	SHADOWGRP
			|| ((sflg && gflg) && (gshadow = sgetsgent (buf)))
#endif
		)) {
			fprintf (stderr, _("%s: error parsing line \"%s\"\n"), Progname, buf);
			errors++;
			continue;
		}
		if (vflg) {
			if (!sflg && pflg) printf (_("adding record for name "%s"\n"), passwd->pw_name);
#ifdef	SHADOWPWD
			if (sflg && pflg) printf (_("adding record for name "%s"\n"), shadow->sp_namp);
#endif
			if (!sflg && gflg) printf (_("adding record for name "%s"\n"), group->gr_name);
#ifdef	SHADOWGRP
			if (sflg && gflg) printf (_("adding record for name "%s"\n"), gshadow->sg_name);
#endif
		}
		if (! sflg && pflg && ! pw_dbm_update (passwd))
			fprintf (stderr, _("%s: error adding record for "%s"\n"),
				Progname, passwd->pw_name);

#ifdef	SHADOWPWD
		if (sflg && pflg && ! sp_dbm_update (shadow))
			fprintf (stderr, _("%s: error adding record for "%s"\n"),
				Progname, shadow->sp_namp);
#endif
		if (! sflg && gflg && ! gr_dbm_update (group))
			fprintf (stderr, _("%s: error adding record for "%s"\n"),
				Progname, group->gr_name);
#ifdef	SHADOWGRP
		if (sflg && gflg && ! sg_dbm_update (gshadow))
			fprintf (stderr, _("%s: error adding record for "%s"\n"),
				Progname, gshadow->sg_name);
#endif	/* SHADOWGRP */

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
		printf (_("added %d entries, longest was %d\n"), cnt, longest);

	exit (errors);
	/*NOTREACHED*/
}

/*
 * usage - print error message and exit
 */

static void
usage(void)
{
#ifdef	SHADOWPWD
#ifdef	SHADOWGRP
	fprintf (stderr, _("Usage: %s [ -vf ] [ -p|g|sp|sg ] file\n"), Progname);
#else	/* !SHADOWGRP */
	fprintf (stderr, _("Usage: %s [ -vf ] [ -p|g|sp ] file\n"), Progname);
#endif	/* SHADOWGRP */
#else	/* !SHADOWPWD */
	fprintf (stderr, _("Usage: %s [ -vf ] [ -p|g ] file\n"), Progname);
#endif	/* SHADOWPWD */


	exit (1);
	/*NOTREACHED*/
}
#endif /*} defined(NDBM) */
