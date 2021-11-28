/*
 * Copyright (c) 1991 - 1993, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id$"

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include "defines.h"
#include "prototypes.h"
#include "shadowlog.h"
/*
 * Global variables
 */
const char *Prog;

/* local function prototypes */
static void print_groups (const char *member);

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
	bool flag = false;

	pwd = getpwnam (member); /* local, no need for xgetpwnam */
	if (NULL == pwd) {
		(void) fprintf (stderr, _("%s: unknown user %s\n"),
		                Prog, member);
		exit (EXIT_FAILURE);
	}

	setgrent ();
	while ((grp = getgrent ()) != NULL) {
		if (is_on_list (grp->gr_mem, member)) {
			if (0 != groups) {
				(void) putchar (' ');
			}
			groups++;

			(void) printf ("%s", grp->gr_name);
			if (grp->gr_gid == pwd->pw_gid) {
				flag = true;
			}
		}
	}
	endgrent ();

	/* The user may not be in the list of members of its primary group */
	if (!flag) {
		grp = getgrgid (pwd->pw_gid); /* local, no need for xgetgrgid */
		if (NULL != grp) {
			if (0 != groups) {
				(void) putchar (' ');
			}
			groups++;

			(void) printf ("%s", grp->gr_name);
		}
	}

	if (0 != groups) {
		(void) putchar ('\n');
	}
}

/*
 * groups - print out the groups a process is a member of
 */
int main (int argc, char **argv)
{
#ifdef HAVE_GETGROUPS
	long sys_ngroups;
	GETGROUPS_T *groups;
#else
	char *logname;
	char *getlogin ();
#endif

#ifdef HAVE_GETGROUPS
	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	groups = (GETGROUPS_T *) malloc (sizeof (GETGROUPS_T) * sys_ngroups);
#endif
	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	/*
	 * Get the program name so that error messages can use it.
	 */
	Prog = Basename (argv[0]);
	log_set_progname(Prog);
	log_set_logfd(stderr);

	if (argc == 1) {

		/*
		 * Called with no arguments - give the group set for the
		 * current user.
		 */

#ifdef HAVE_GETGROUPS
		int i;
		int pri_grp; /* TODO: should be GETGROUPS_T */
		/*
		 * This system supports concurrent group sets, so I can ask
		 * the system to tell me which groups are currently set for
		 * this process.
		 */
		int ngroups = getgroups (sys_ngroups, groups);
		if (ngroups < 0) {
			perror ("getgroups");
			exit (EXIT_FAILURE);
		}

		/*
		 * The groupset includes the primary group as well.
		 */
		pri_grp = getegid ();
		for (i = 0; i < ngroups; i++) {
			if (pri_grp == (int) groups[i]) {
				break;
			}
		}

		if (i != ngroups) {
			pri_grp = -1;
		}

		/*
		 * Print out the name of every group in the current group
		 * set. Unknown groups are printed as their decimal group ID
		 * values.
		 */
		if (-1 != pri_grp) {
			struct group *gr;
			/* local, no need for xgetgrgid */
			gr = getgrgid (pri_grp);
			if (NULL != gr) {
				(void) printf ("%s", gr->gr_name);
			} else {
				(void) printf ("%d", pri_grp);
			}
		}

		for (i = 0; i < ngroups; i++) {
			struct group *gr;
			if ((0 != i) || (-1 != pri_grp)) {
				(void) putchar (' ');
			}

			/* local, no need for xgetgrgid */
			gr = getgrgid (groups[i]);
			if (NULL != gr) {
				(void) printf ("%s", gr->gr_name);
			} else {
				(void) printf ("%ld", (long) groups[i]);
			}
		}
		(void) putchar ('\n');
#else
		/*
		 * This system does not have the getgroups() system call, so
		 * I must check the groups file directly.
		 */
		logname = getlogin ();
		if (NULL != logname) {
			print_groups (logname);
		} else {
			exit (EXIT_FAILURE);
		}
#endif
	} else {

		/*
		 * The invoker wanted to know about some other user. Use
		 * that name to look up the groups instead.
		 */
		print_groups (argv[1]);
	}
	return EXIT_SUCCESS;
}

