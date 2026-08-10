/*
 * Copyright 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

static	char	sccsid[] = "@(#)groups.c	3.2	09:47:19	25 Nov 1991";

#include "stdio.h"
#include "pwd.h"
#include "grp.h"

/*
 * print_groups - print the groups which the named user is a member of
 *
 *	print_groups() scans the groups file for the list of groups
 *	which the user is listed as being a member of.
 */

print_groups (member)
char	*member;
{
	int	i, groups = 0;
	struct	group	*grp;
	struct	group	*getgrent();

	setgrent ();

	while (grp = getgrent ()) {
		for (i = 0;grp->gr_mem[i];i++) {
			if (strcmp (grp->gr_mem[i], member) == 0) {
				if (groups++)
					putchar (' ');

				printf ("%s", grp->gr_name);
			}
		}
	}
	if (groups)
		putchar ('\n');
}

/*
 * groups - print out the groups a process is a member of
 */

main (argc, argv)
int	argc;
char	**argv;
{
	int	ngroups;
#if NGROUPS > 0
#if NGROUPS > 100
	gid_t	*groups;
#else
	gid_t	groups[NGROUPS];
#endif
	int	i;
#else
	char	*logname;
	char	*getlogin();
#endif
	struct	group	*gr;
	struct	group	*getgrgid();

	if (argc == 1) {

		/*
		 * Called with no arguments - give the group set
		 * for the current user.
		 */

#if NGROUPS > 0
		/*
		 * This system supports concurrent group sets, so
		 * I can ask the system to tell me which groups are
		 * currently set for this process.
		 */

		ngroups = getgroups (0, 0);
#if NGROUPS > 100
		groups = (gid_t *) malloc (ngroups * sizeof (int *));
#endif
		getgroups (ngroups, groups);

		/*
		 * Print out the name of every group in the current
		 * group set.  Unknown groups are printed as their
		 * decimal group ID values.
		 */

		for (i = 0;i < ngroups;i++) {
			if (i)
				putchar (' ');

			if (gr = getgrgid (groups[i]))
				printf ("%s", gr->gr_name);
			else
				printf ("%d", groups[i]);
		}
		putchar ('\n');
#else
		/*
		 * This system does not have the getgroups() system
		 * call, so I must check the groups file directly.
		 */

		if (logname = getlogin ())
			print_groups (logname);
		else
			exit (1);
#endif
	} else {

		/*
		 * The invoker wanted to know about some other
		 * user.  Use that name to look up the groups instead.
		 */

		if (getpwnam (argv[1]) == 0) {
			fprintf (stderr, "unknown user %s\n", argv[1]);
			exit (1);
		}
		print_groups (argv[1]);
	}
	exit (0);
}
