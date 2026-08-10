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

/*
 * id - print current process user identification information
 *
 *	Print the current process identifiers.  This includes the
 *	UID, GID, effective-UID and effective-GID.  Optionally print
 *	the concurrent group set if the current system supports it.
 */

#include <sys/types.h>
#include <stdio.h>
#include <grp.h>
#include "config.h"
#include "pwd.h"

#ifndef	lint
static	char	sccsid[] = "@(#)id.c	3.7	07:17:31	03 Jun 1993";
#endif

usage ()
{
#if NGROUPS > 0
	fprintf (stderr, "usage: id [ -a ]\n");
#else
	fprintf (stderr, "usage: id\n");
#endif
	exit (1);
}

/*ARGSUSED*/
main (argc, argv)
int	argc;
char	**argv;
{
	int	id;
#if NGROUPS > 0
#if NGROUPS > 100
	GID_T	*groups;
#else
	GID_T	groups[NGROUPS];
#endif
	int	ngroups;
	int	aflg = 0;
#endif
	struct	passwd	*pw,
			*getpwuid();
	struct	group	*gr,
			*getgrgid();

#if NGROUPS > 0
	/*
	 * See if the -a flag has been given to print out the
	 * concurrent group set.
	 */

	if (argc > 1) {
		if (argc > 2 || strcmp (argv[1], "-a"))
			usage ();
		else
			aflg = 1;
	}
#else
	if (argc > 1)
		usage ();
#endif

	/*
	 * Print out the real user ID and group ID.  If the user or
	 * group does not exist, just give the numerical value.
	 */

	if (pw = getpwuid (id = getuid ()))
		printf ("uid=%d(%s)", id, pw->pw_name);
	else
		printf ("uid=%d", id);

	if (gr = getgrgid (id = getgid ()))
		printf (" gid=%d(%s)", id, gr->gr_name);
	else
		printf (" gid=%d", id);

	/*
	 * Print out the effective user ID and group ID if they are
	 * different from the real values.
	 */

	if (getuid () != geteuid ()) {
		if (pw = getpwuid (id = geteuid ()))
			printf (" euid=%d(%s)", id, pw->pw_name);
		else
			printf (" euid=%d", id);
	}
	if (getgid () != getegid ()) {
		if (gr = getgrgid (id = getegid ()))
			printf (" egid=%d(%s)", id, gr->gr_name);
		else
			printf (" egid=%d", id);
	}
#if NGROUPS > 0

	/*
	 * Print out the concurrent group set if the user has requested
	 * it.  The group numbers will be printed followed by their
	 * names.
	 */

	if (aflg && (ngroups = getgroups (0, 0)) != -1) {
		int	i;

#if NGROUPS > 100
		/*
		 * The size of the group set is determined so an array
		 * large enough to hold it can be allocated.
		 */

		if (groups = (int *) malloc (ngroups * sizeof *groups)) {
			putchar ('\n');
			perror ("out of memory");
			exit (1);
		}
#endif
		/*
		 * Start off the group message.  It will be of the format
		 *
		 *	groups=###(aaa),###(aaa),###(aaa)
		 *
		 * where "###" is a numerical value and "aaa" is the
		 * corresponding name for each respective numerical value.
		 */

		getgroups (ngroups, groups);
		printf (" groups=");
		for (i = 0;i < ngroups;i++) {
			if (i)
				putchar (',');

			if (gr = getgrgid (groups[i]))
				printf ("%d(%s)", (int) groups[i], gr->gr_name);
			else
				printf ("%d", (int) groups[i]);
		}
	}
#endif

	/*
	 * Finish off the line.
	 */

	putchar ('\n');
	exit (0);
	/*NOTREACHED*/
}
