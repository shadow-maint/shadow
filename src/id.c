/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
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

/*
 * id - print current process user identification information
 *
 *	Print the current process identifiers. This includes the
 *	UID, GID, effective-UID and effective-GID. Optionally print
 *	the concurrent group set if the current system supports it.
 */

#include <config.h>

#ident "$Id$"

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
/* local function prototypes */
static void usage (void);

static void usage (void)
{
#ifdef HAVE_GETGROUPS
	(void) fputs (_("Usage: id [-a]\n"), stderr);
#else
	(void) fputs (_("Usage: id\n"), stderr);
#endif
	exit (EXIT_FAILURE);
}

 /*ARGSUSED*/ int main (int argc, char **argv)
{
	uid_t ruid, euid;
	gid_t rgid, egid;
	int i;
	long sys_ngroups;

/*
 * This block of declarations is particularly strained because of several
 * different ways of doing concurrent groups. Old BSD systems used int for
 * gid's, but short for the type passed to getgroups(). Newer systems use
 * gid_t for everything. Some systems have a small and fixed NGROUPS,
 * usually about 16 or 32. Others use bigger values.
 */
#ifdef HAVE_GETGROUPS
	GETGROUPS_T *groups;
	int ngroups;
	bool aflg = 0;
#endif
	struct passwd *pw;
	struct group *gr;

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	/*
	 * Dynamically get the maximum number of groups from system, instead
	 * of using the symbolic constant NGROUPS_MAX. This ensures that the
	 * group limit is not hard coded into the binary, so it will still
	 * work if the system library is recompiled.
	 */
	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
#ifdef HAVE_GETGROUPS
	groups = (GETGROUPS_T *) malloc (sizeof (GETGROUPS_T) * sys_ngroups);
	/*
	 * See if the -a flag has been given to print out the concurrent
	 * group set.
	 */

	if (argc > 1) {
		if ((argc > 2) || (strcmp (argv[1], "-a") != 0)) {
			usage ();
		} else {
			aflg = true;
		}
	}
#else
	if (argc > 1) {
		usage ();
	}
#endif

	ruid = getuid ();
	euid = geteuid ();
	rgid = getgid ();
	egid = getegid ();

	/*
	 * Print out the real user ID and group ID. If the user or group
	 * does not exist, just give the numerical value.
	 */

	pw = getpwuid (ruid); /* local, no need for xgetpwuid */
	if (NULL != pw) {
		(void) printf ("UID=%lu(%s)",
		               (unsigned long) ruid, pw->pw_name);
	} else {
		(void) printf ("UID=%lu", (unsigned long) ruid);
	}

	gr = getgrgid (rgid);; /* local, no need for xgetgrgid */
	if (NULL != gr) {
		(void) printf (" GID=%lu(%s)",
		               (unsigned long) rgid, gr->gr_name);
	} else {
		(void) printf (" GID=%lu", (unsigned long) rgid);
	}

	/*
	 * Print out the effective user ID and group ID if they are
	 * different from the real values.
	 */

	if (ruid != euid) {
		pw = getpwuid (euid); /* local, no need for xgetpwuid */
		if (NULL != pw) {
			(void) printf (" EUID=%lu(%s)",
			               (unsigned long) euid, pw->pw_name);
		} else {
			(void) printf (" EUID=%lu", (unsigned long) euid);
		}
	}
	if (rgid != egid) {
		gr = getgrgid (egid); /* local, no need for xgetgrgid */
		if (NULL != gr) {
			(void) printf (" EGID=%lu(%s)",
			               (unsigned long) egid, gr->gr_name);
		} else {
			(void) printf (" EGID=%lu", (unsigned long) egid);
		}
	}
#ifdef HAVE_GETGROUPS
	/*
	 * Print out the concurrent group set if the user has requested it.
	 * The group numbers will be printed followed by their names.
	 */
	if (aflg && (ngroups = getgroups (sys_ngroups, groups)) != -1) {

		/*
		 * Start off the group message. It will be of the format
		 *
		 *      groups=###(aaa),###(aaa),###(aaa)
		 *
		 * where "###" is a numerical value and "aaa" is the
		 * corresponding name for each respective numerical value.
		 */
		(void) puts (_(" groups="));
		for (i = 0; i < ngroups; i++) {
			if (0 != i)
				(void) putchar (',');

			/* local, no need for xgetgrgid */
			gr = getgrgid (groups[i]);
			if (NULL != gr) {
				(void) printf ("%lu(%s)",
				               (unsigned long) groups[i],
				               gr->gr_name);
			} else {
				(void) printf ("%lu",
				               (unsigned long) groups[i]);
			}
		}
	}
	free (groups);
#endif

	/*
	 * Finish off the line.
	 */
	(void) putchar ('\n');

	return EXIT_SUCCESS;
}

