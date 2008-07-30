/*
 * Copyright (c) 2000       , International Business Machines
 *                            George Kraft IV, gk4@us.ibm.com, 03/23/2000
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
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

#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include <pwd.h>
#include "defines.h"
#include "prototypes.h"
#include "groupio.h"

/* Exit Status Values */

#define EXIT_SUCCESS		0	/* success */
#define EXIT_USAGE		1	/* invalid command syntax */
#define EXIT_GROUP_FILE		2	/* group file access problems */
#define EXIT_NOT_ROOT		3	/* not superuser  */
#define EXIT_NOT_EROOT		4	/* not effective superuser  */
#define EXIT_NOT_PRIMARY	5	/* not primary owner of group  */
#define EXIT_NOT_MEMBER		6	/* member of group does not exist */
#define EXIT_MEMBER_EXISTS	7	/* member of group already exists */
#define EXIT_INVALID_USER	8	/* specified user does not exist */
#define EXIT_INVALID_GROUP	9	/* specified group does not exist */

/*
 * Global variables
 */
static char *adduser = NULL;
static char *deluser = NULL;
static char *thisgroup = NULL;
static bool purge = false;
static bool list = false;
static int exclusive = 0;
static char *Prog;
static bool group_locked = false;

static char *whoami (void);
static void members (char **members);
static void usage (void);
static void process_flags (int argc, char **argv);
static void check_perms (void);
static void fail_exit (int code);
#define isroot()		(getuid () == 0)

static char *whoami (void)
{
	/* local, no need for xgetgrgid */
	struct group *grp = getgrgid (getgid ());
	/* local, no need for xgetpwuid */
	struct passwd *usr = getpwuid (getuid ());

	if (   (NULL != usr)
	    && (NULL != grp)
	    && (0 == strcmp (usr->pw_name, grp->gr_name))) {
		return xstrdup (usr->pw_name);
	} else {
		return NULL;
	}
}

static void members (char **members)
{
	int i;

	for (i = 0; NULL != members[i]; i++) {
		printf ("%s ", members[i]);

		if (NULL == members[i + 1]) {
			printf ("\n");
		} else {
			printf (" ");
		}
	}
}

static void usage (void)
{
	(void) fputs (_("Usage: groupmems -a username | -d username | -p | -l [-g groupname]\n"), stderr);
	fail_exit (EXIT_USAGE);
}

/*
 * process_flags - perform command line argument setting
 */
static void process_flags (int argc, char **argv)
{
	int arg;
	int option_index = 0;
	static struct option long_options[] = {
		{"add", required_argument, NULL, 'a'},
		{"delete", required_argument, NULL, 'd'},
		{"group", required_argument, NULL, 'g'},
		{"list", no_argument, NULL, 'l'},
		{"purge", no_argument, NULL, 'p'},
		{NULL, 0, NULL, '\0'}
	};

	while ((arg = getopt_long (argc, argv, "a:d:g:lp", long_options,
	                           &option_index)) != EOF) {
		switch (arg) {
		case 'a':
			adduser = xstrdup (optarg);
			++exclusive;
			break;
		case 'd':
			deluser = xstrdup (optarg);
			++exclusive;
			break;
		case 'p':
			purge = true;
			++exclusive;
			break;
		case 'g':
			thisgroup = xstrdup (optarg);
			break;
		case 'l':
			list = true;
			++exclusive;
			break;
		default:
			usage ();
		}
	}

	if ((exclusive > 1) || (optind < argc)) {
		usage ();
	}

	/* local, no need for xgetpwnam */
	if (   (NULL != adduser)
	    && (getpwnam (adduser) == NULL)) {
		fprintf (stderr, _("%s: user `%s' does not exist\n"),
		         Prog, adduser);
		fail_exit (EXIT_INVALID_USER);
	}

}

static void check_perms (void)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval = PAM_SUCCESS;
	struct passwd *pampw;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (NULL == pampw) {
		retval = PAM_USER_UNKNOWN;
	} else {
		retval = pam_start ("groupmod", pampw->pw_name,
		                    &conv, &pamh);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_authenticate (pamh, 0);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_acct_mgmt (pamh, 0);
	}

	(void) pam_end (pamh, retval);
	if (PAM_SUCCESS != retval) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		fail_exit (1);
	}
#endif
}

static void fail_exit (int code)
{
	if (group_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr,
			         _("%s: unable to unlock group file\n"),
			         Prog);
		}
	}
	exit (code);
}

void main (int argc, char **argv) 
{
	char *name;
	struct group *grp;

	/*
	 * Get my name so that I can use it to report errors.
	 */
	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_flags (argc, argv);

	if (NULL == thisgroup) {
		name = whoami ();
		if (!list && (NULL == name)) {
			fprintf (stderr, _("%s: your groupname does not match your username\n"), Prog);
			fail_exit (EXIT_NOT_PRIMARY);
		}
	} else {
		name = thisgroup;
		if (!list && !isroot ()) {
			fprintf (stderr, _("%s: only root can use the -g/--group option\n"), Prog);
			fail_exit (EXIT_NOT_ROOT);
		}
	}

	if (!list) {
		check_perms ();

		if (gr_lock () == 0) {
			fprintf (stderr,
			         _("%s: unable to lock group file\n"), Prog);
			fail_exit (EXIT_GROUP_FILE);
		}
		group_locked = true;
	}

	if (gr_open (list ? O_RDONLY : O_RDWR) == 0) {
		fprintf (stderr, _("%s: unable to open group file\n"), Prog);
		fail_exit (EXIT_GROUP_FILE);
	}

	grp = (struct group *) gr_locate (name);

	if (NULL == grp) {
		fprintf (stderr, _("%s: `%s' not found in /etc/group\n"),
		         Prog, name);
		fail_exit (EXIT_INVALID_GROUP);
	}

	if (list) {
		members (grp->gr_mem);
	} else if (NULL != adduser) {
		if (is_on_list (grp->gr_mem, adduser)) {
			fprintf (stderr,
			         _("%s: user `%s' is already a member of `%s'\n"),
			         Prog, adduser, grp->gr_name);
			fail_exit (EXIT_MEMBER_EXISTS);
		}
		grp->gr_mem = add_list (grp->gr_mem, adduser);
		gr_update (grp);
	} else if (NULL != deluser) {
		if (!is_on_list (grp->gr_mem, adduser)) {
			fprintf (stderr,
			         _("%s: user `%s' is not a member of `%s'\n"),
			         Prog, deluser, grp->gr_name);
			fail_exit (EXIT_NOT_MEMBER);
		}
		grp->gr_mem = del_list (grp->gr_mem, deluser);
		gr_update (grp);
	} else if (purge) {
		grp->gr_mem[0] = NULL;
		gr_update (grp);
	}

	if (gr_close () == 0) {
		fprintf (stderr, _("%s: unable to close group file\n"), Prog);
		fail_exit (EXIT_GROUP_FILE);
	}

	fail_exit (EXIT_SUCCESS);
}

