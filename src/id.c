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
	fputs (_("Usage: id [-a]\n"), stderr);
#else
	fputs (_("Usage: id\n"), stderr);
#endif
	exit (1);
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
	int aflg = 0;
#endif
	struct passwd *pw;
	struct group *gr;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * Dynamically get the maximum number of groups from system, instead
	 * of using the symbolic constant NGROUPS_MAX. This ensures that the
	 * group limit is not hard coded into the binary, so it will still
	 * work if the system library is recompiled.
	 */
	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
#ifdef HAVE_GETGROUPS
	groups = (GETGROUPS_T *) malloc (sys_ngroups * sizeof (GETGROUPS_T));
	/*
	 * See if the -a flag has been given to print out the concurrent
	 * group set.
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

	ruid = getuid ();
	euid = geteuid ();
	rgid = getgid ();
	egid = getegid ();

	/*
	 * Print out the real user ID and group ID. If the user or group
	 * does not exist, just give the numerical value.
	 */

	pw = getpwuid (ruid); /* local, no need for xgetpwuid */
	if (pw)
		printf ("UID=%u(%s)", ruid, pw->pw_name);
	else
		printf ("UID=%u", ruid);

	gr = getgrgid (rgid);; /* local, no need for xgetgrgid */
	if (gr)
		printf (" GID=%u(%s)", rgid, gr->gr_name);
	else
		printf (" GID=%u", rgid);

	/*
	 * Print out the effective user ID and group ID if they are
	 * different from the real values.
	 */

	if (ruid != euid) {
		pw = getpwuid (euid); /* local, no need for xgetpwuid */
		if (pw)
			printf (" EUID=%u(%s)", euid, pw->pw_name);
		else
			printf (" EUID=%u", euid);
	}
	if (rgid != egid) {
		gr = getgrgid (egid); /* local, no need for xgetgrgid */
		if (gr)
			printf (" EGID=%u(%s)", egid, gr->gr_name);
		else
			printf (" EGID=%u", egid);
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
		puts (_(" groups="));
		for (i = 0; i < ngroups; i++) {
			if (i)
				putchar (',');

			/* local, no need for xgetgrgid */
			gr = getgrgid (groups[i]);
			if (gr)
				printf ("%u(%s)", groups[i], gr->gr_name);
			else
				printf ("%u", groups[i]);
		}
	}
	free (groups);
#endif

	/*
	 * Finish off the line.
	 */
	putchar ('\n');
	exit (0);
	/* NOT REACHED */
}

