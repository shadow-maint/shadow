/*
 * Copyright 1991, 1992, 1993, John F. Haugh II
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

#ifndef lint
static	char	sccsid[] = "@(#)groupadd.c	3.6	08:09:24	23 Apr 1993";
#endif

#include <sys/types.h>
#include <stdio.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "config.h"
#include "shadow.h"

#ifdef	USE_SYSLOG
#include <syslog.h>
#endif

char	group_name[BUFSIZ];
GID_T	group_id;

char	*Prog;

int	oflg;	/* permit non-unique group ID to be specified with -g         */
int	gflg;	/* ID value for the new group                                 */

#ifdef	NDBM
extern	int	gr_dbm_mode;
extern	int	sg_dbm_mode;
#endif
extern	char	*malloc();

extern	struct	group	*getgrnam();
extern	struct	group	*gr_next();
extern	int	gr_lock();
extern	int	gr_unlock();
extern	int	gr_rewind();
extern	int	gr_open();

#ifdef	SHADOWGRP
extern	int	sgr_lock();
extern	int	sgr_unlock();
extern	int	sgr_open();
#endif

/*
 * usage - display usage message and exit
 */

usage ()
{
	fprintf (stderr, "usage: groupadd [-g gid [-o]] group\n");
	exit (2);
}

/*
 * new_grent - initialize the values in a group file entry
 *
 *	new_grent() takes all of the values that have been entered and
 *	fills in a (struct group) with them.
 */

void
new_grent (grent)
struct	group	*grent;
{
	static	char	*empty_list = 0;

	memset (grent, 0, sizeof *grent);
	grent->gr_name = group_name;
	grent->gr_passwd = "*";
	grent->gr_gid = group_id;
	grent->gr_mem = &empty_list;
}

#ifdef	SHADOWGRP
/*
 * new_sgent - initialize the values in a shadow group file entry
 *
 *	new_sgent() takes all of the values that have been entered and
 *	fills in a (struct sgrp) with them.
 */

void
new_sgent (sgent)
struct	sgrp	*sgent;
{
	static	char	*empty_list = 0;

	memset (sgent, 0, sizeof *sgent);
	sgent->sg_name = group_name;
	sgent->sg_passwd = "!";
	sgent->sg_adm = &empty_list;
	sgent->sg_mem = &empty_list;
}
#endif	/* SHADOWGRP */

/*
 * grp_update - add new group file entries
 *
 *	grp_update() writes the new records to the group files.
 */

void
grp_update ()
{
	struct	group	grp;
#ifdef	SHADOWGRP
	struct	sgrp	sgrp;
#endif	/* SHADOWGRP */

	/*
	 * Create the initial entries for this new group.
	 */

	new_grent (&grp);
#ifdef	SHADOWGRP
	new_sgent (&sgrp);
#endif	/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */

	if (! gr_update (&grp)) {
		fprintf (stderr, "%s: error adding new group entry\n", Prog);
		fail_exit (1);
	}
#ifdef	NDBM

	/*
	 * Update the DBM group file with the new entry as well.
	 */

	if (access ("/etc/group.pag", 0) == 0 && ! gr_dbm_update (&grp)) {
		fprintf (stderr, "%s: cannot add new dbm group entry\n", Prog);
		fail_exit (1);
	}
	endgrent ();
#endif	/* NDBM */

#ifdef	SHADOWGRP

	/*
	 * Write out the new shadow group entries as well.
	 */

	if (! sgr_update (&sgrp)) {
		fprintf (stderr, "%s: error adding new group entry\n", Prog);
		fail_exit (1);
	}
#ifdef	NDBM

	/*
	 * Update the DBM group file with the new entry as well.
	 */

	if (access ("/etc/gshadow.pag", 0) == 0 && ! sg_dbm_update (&sgrp)) {
		fprintf (stderr, "%s: cannot add new dbm group entry\n", Prog);
		fail_exit (1);
	}
	endsgent ();
#endif	/* NDBM */
#endif	/* SHADOWGRP */
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, "new group: name=%s, gid=%d\n",
		group_name, group_id);
#endif	/* USE_SYSLOG */
}

/*
 * find_new_gid - find the next available GID
 *
 *	find_new_gid() locates the next highest unused GID in the group
 *	file, or checks the given group ID against the existing ones for
 *	uniqueness.
 */

void
find_new_gid ()
{
	struct	group	*grp;

	/*
	 * Start with some GID value if the user didn't provide us with
	 * one already.
	 */

	if (! gflg)
		group_id = 100;

	/*
	 * Search the entire group file, either looking for this
	 * GID (if the user specified one with -g) or looking for the
	 * largest unused value.
	 */

	for (gr_rewind (), grp = gr_next ();grp;grp = gr_next ()) {
		if (strcmp (group_name, grp->gr_name) == 0) {
			fprintf (stderr, "%s: name %s is not unique\n",
				Prog, group_name);
			fail_exit (1);
		}
		if (gflg && group_id == grp->gr_gid) {
			fprintf (stderr, "%s: gid %d is not unique\n",
				Prog, group_id);
			fail_exit (1);
		}
		if (! gflg && grp->gr_gid >= group_id)
			group_id = grp->gr_gid + 1;
	}
}

/*
 * check_new_name - check the new name for validity
 *
 *	check_new_name() insures that the new name doesn't contain
 *	any illegal characters.
 */

void
check_new_name ()
{
	int	i;

	/*
	 * Check for validity.  The name must be at least 1 character in
	 * length, but not more than 10.  It must start with a letter and
	 * contain printable characters, not including ':' and '\n'
	 */

	if (strlen (group_name) > 10)
		goto bad_name;

	if (strlen (group_name) >= 1 && isalpha (group_name[0])) {
		for (i = 1;group_name[i] && isprint (group_name[i]);i++)
			if (group_name[i] == ':' ||
					group_name[i] == '\n')
				goto bad_name;

		/*
		 * Something was left over.  It must be a non-printable
		 * character.
		 */

		if (group_name[i])
			goto bad_name;

		return;
	}

	/*
	 * All invalid group names land here.
	 */

bad_name:
	fprintf (stderr, "%s: %s is a not a valid group name\n",
		Prog, group_name);

	exit (3);
}

/*
 * process_flags - perform command line argument setting
 *
 *	process_flags() interprets the command line arguments and sets
 *	the values that the user will be created with accordingly.  The
 *	values are checked for sanity.
 */

void
process_flags (argc, argv)
int	argc;
char	**argv;
{
	extern	int	optind;
	extern	char	*optarg;
	char	*end;
	int	arg;

	while ((arg = getopt (argc, argv, "og:")) != EOF) {
		switch (arg) {
			case 'g':
				gflg++;
				if (! isdigit (optarg[0]))
					usage ();

				group_id = strtol (optarg, &end, 10);
				if (*end != '\0') {
					fprintf (stderr, "%s: invalid group %s\n",
						Prog, optarg);
					fail_exit (3);
				}
				break;
			case 'o':
				if (! gflg)
					usage ();

				oflg++;
				break;
			default:
				usage ();
		}
	}
	if (optind == argc - 1)
		strcpy (group_name, argv[argc - 1]);
	else
		usage ();

	check_new_name ();
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new group.  This causes any modified entries to be written out.
 */

close_files ()
{
	if (! gr_close ()) {
		fprintf (stderr, "%s: cannot rewrite group file\n", Prog);
		fail_exit (1);
	}
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (! sgr_close ()) {
		fprintf (stderr, "%s: cannot rewrite shadow group file\n",
			Prog);
		fail_exit (1);
	}
	(void) sgr_unlock ();
#endif	/* SHADOWGRP */
}

/*
 * open_files - lock and open the group files
 *
 *	open_files() opens the two group files.
 */

open_files ()
{
	if (! gr_lock ()) {
		fprintf (stderr, "%s: unable to lock group file\n", Prog);
		exit (1);
	}
	if (! gr_open (O_RDWR)) {
		fprintf (stderr, "%s: unable to open group file\n", Prog);
		fail_exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_lock ()) {
		fprintf (stderr, "%s: unable to lock shadow group file\n",
			Prog);
		fail_exit (1);
	}
	if (! sgr_open (O_RDWR)) {
		fprintf (stderr, "%s: unable to open shadow group file\n",
			Prog);
		fail_exit (1);
	}
#endif	/* SHADOWGRP */
}

/*
 * fail_exit - exit with an error code after unlocking files
 */

fail_exit (code)
int	code;
{
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	(void) sgr_unlock ();
#endif
	exit (code);
}

/*
 * main - useradd command
 */

main (argc, argv)
int	argc;
char	**argv;
{

	/*
	 * Get my name so that I can use it to report errors.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif	/* USE_SYSLOG */

	/*
	 * The open routines for the DBM files don't use read-write
	 * as the mode, so we have to clue them in.
	 */

#ifdef	NDBM
	gr_dbm_mode = O_RDWR;
#ifdef	SHADOWGRP
	sg_dbm_mode = O_RDWR;
#endif	/* SHADOWGRP */
#endif	/* NDBM */
	process_flags (argc, argv);

	/*
	 * Start with a quick check to see if the group exists.
	 */

	if (getgrnam (group_name)) {
		fprintf (stderr, "%s: group %s exists\n", Prog, group_name);
		exit (9);
	}

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */

	open_files ();

	if (! gflg || ! oflg)
		find_new_gid ();

	grp_update ();

	close_files ();
	exit (0);
	/*NOTREACHED*/
}
