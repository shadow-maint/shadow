/*
 * Copyright 1991 - 1993, Julianne Frances Haugh
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
 * ARE DISCLAIMED. IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id$"

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include "defines.h"
#include "prototypes.h"
/*
 * Global variables
 */
static char *Prog;

/* local function prototypes */
static void print_groups (const char *);

/*
 * print_groups - print the groups which the named user is a member of
 *
 *	print_groups() scans the groups file for the list of groups which
 *	the user is listed as being a member of.
 */
static void print_groups (const char *member)
{
	int groups = 0;
	struct group *grp;
	struct passwd *pwd;
	int flag = 0;

	/* local, no need for xgetpwnam */
	if ((pwd = getpwnam (member)) == 0) {
		fprintf (stderr, _("%s: unknown user %s\n"), Prog, member);
		exit (1);
	}
	setgrent ();
	while ((grp = getgrent ())) {
		if (is_on_list (grp->gr_mem, member)) {
			if (groups++)
				putchar (' ');

			printf ("%s", grp->gr_name);
			if (grp->gr_gid == pwd->pw_gid)
				flag = 1;
		}
	}
	endgrent ();
	/* local, no need for xgetgrgid */
	if (!flag && (grp = getgrgid (pwd->pw_gid))) {
		if (groups++)
			putchar (' ');

		printf ("%s", grp->gr_name);
	}
	if (groups)
		putchar ('\n');
}

/*
 * groups - print out the groups a process is a member of
 */
int main (int argc, char **argv)
{
	long sys_ngroups;

#ifdef HAVE_GETGROUPS
	int ngroups;
	GETGROUPS_T *groups;
	int pri_grp;
	int i;
#else
	char *logname;
	char *getlogin ();
#endif

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
#ifdef HAVE_GETGROUPS
	groups = (GETGROUPS_T *) malloc (sys_ngroups * sizeof (GETGROUPS_T));
#endif
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * Get the program name so that error messages can use it.
	 */
	Prog = Basename (argv[0]);

	if (argc == 1) {

		/*
		 * Called with no arguments - give the group set for the
		 * current user.
		 */

#ifdef HAVE_GETGROUPS
		/*
		 * This system supports concurrent group sets, so I can ask
		 * the system to tell me which groups are currently set for
		 * this process.
		 */
		ngroups = getgroups (sys_ngroups, groups);
		if (ngroups < 0) {
			perror ("getgroups");
			exit (1);
		}

		/*
		 * The groupset includes the primary group as well.
		 */
		pri_grp = getegid ();
		for (i = 0; i < ngroups; i++)
			if (pri_grp == (int) groups[i])
				break;

		if (i != ngroups)
			pri_grp = -1;

		/*
		 * Print out the name of every group in the current group
		 * set. Unknown groups are printed as their decimal group ID
		 * values.
		 */
		if (pri_grp != -1) {
			struct group *gr;
			/* local, no need for xgetgrgid */
			if ((gr = getgrgid (pri_grp)))
				printf ("%s", gr->gr_name);
			else
				printf ("%d", pri_grp);
		}

		for (i = 0; i < ngroups; i++) {
			struct group *gr;
			if (i || pri_grp != -1)
				putchar (' ');

			/* local, no need for xgetgrgid */
			if ((gr = getgrgid (groups[i])))
				printf ("%s", gr->gr_name);
			else
				printf ("%ld", (long) groups[i]);
		}
		putchar ('\n');
#else
		/*
		 * This system does not have the getgroups() system call, so
		 * I must check the groups file directly.
		 */
		if ((logname = getlogin ()))
			print_groups (logname);
		else
			exit (1);
#endif
	} else {

		/*
		 * The invoker wanted to know about some other user. Use
		 * that name to look up the groups instead.
		 */
		print_groups (argv[1]);
	}
	exit (0);
}
