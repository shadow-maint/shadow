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

#include "rcsid.h"
RCSID (PKG_VER "$Id: groupadd.c,v 1.41 2005/08/11 13:45:41 kloczek Exp $")
#include <sys/types.h>
#include <stdio.h>
#include <getopt.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif				/* USE_PAM */
#include "defines.h"
#include "prototypes.h"
#include "chkname.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
static int is_shadow_grp;
#endif

/*
 * exit status values
 */

#define E_SUCCESS	0	/* success */
#define E_USAGE		2	/* invalid command syntax */
#define E_BAD_ARG	3	/* invalid argument to option */
#define E_GID_IN_USE	4	/* gid not unique (when -o not used) */
#define E_NAME_IN_USE	9	/* group name not unique */
#define E_GRP_UPDATE	10	/* can't update group file */

static char *group_name;
static gid_t group_id;
static char *empty_list = NULL;

static char *Prog;

static int oflg = 0;		/* permit non-unique group ID to be specified with -g */
static int gflg = 0;		/* ID value for the new group */
static int fflg = 0;		/* if group already exists, do nothing and exit(0) */

/* local function prototypes */
static void usage (void);
static void new_grent (struct group *);

#ifdef SHADOWGRP
static void new_sgent (struct sgrp *);
#endif
static void grp_update (void);
static void find_new_gid (void);
static void check_new_name (void);
static void process_flags (int, char **);
static void close_files (void);
static void open_files (void);
static void fail_exit (int);

/*
 * usage - display usage message and exit
 */

static void usage (void)
{
	fprintf (stderr, _("Usage: groupadd [options] group\n"
			   "\n"
			   "Options:\n"
			   "  -f, --force 		force exit with success status if the specified\n"
			   "				group already exists\n"
			   "  -g, --gid GID		use GID for the new group\n"
			   "  -h, --help			display this help message and exit\n"
			   "  -K, --key KEY=VALUE		overrides /etc/login.defs defaults\n"
			   "  -o, --non-unique		allow create group with duplicate\n"
			   "				(non-unique) GID\n"));
	exit (E_USAGE);
}

/*
 * new_grent - initialize the values in a group file entry
 *
 *	new_grent() takes all of the values that have been entered and fills
 *	in a (struct group) with them.
 */

static void new_grent (struct group *grent)
{
	memzero (grent, sizeof *grent);
	grent->gr_name = group_name;
	grent->gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
	grent->gr_gid = group_id;
	grent->gr_mem = &empty_list;
}

#ifdef	SHADOWGRP
/*
 * new_sgent - initialize the values in a shadow group file entry
 *
 *	new_sgent() takes all of the values that have been entered and fills
 *	in a (struct sgrp) with them.
 */

static void new_sgent (struct sgrp *sgent)
{
	memzero (sgent, sizeof *sgent);
	sgent->sg_name = group_name;
	sgent->sg_passwd = "!";	/* XXX warning: const */
	sgent->sg_adm = &empty_list;
	sgent->sg_mem = &empty_list;
}
#endif				/* SHADOWGRP */

/*
 * grp_update - add new group file entries
 *
 *	grp_update() writes the new records to the group files.
 */

static void grp_update (void)
{
	struct group grp;

#ifdef	SHADOWGRP
	struct sgrp sgrp;
#endif				/* SHADOWGRP */

	/*
	 * Create the initial entries for this new group.
	 */
	new_grent (&grp);
#ifdef	SHADOWGRP
	new_sgent (&sgrp);
#endif				/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */
	if (!gr_update (&grp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && !sgr_update (&sgrp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u",
		 group_name, (unsigned int) group_id));
}

/*
 * find_new_gid - find the next available GID
 *
 *	find_new_gid() locates the next highest unused GID in the group
 *	file, or checks the given group ID against the existing ones for
 *	uniqueness.
 */

static void find_new_gid (void)
{
	const struct group *grp;
	gid_t gid_min, gid_max;

	gid_min = getdef_unum ("GID_MIN", 100);
	gid_max = getdef_unum ("GID_MAX", 60000);

	/*
	 * Start with some GID value if the user didn't provide us with
	 * one already.
	 */

	if (!gflg)
		group_id = gid_min;

	/*
	 * Search the entire group file, either looking for this GID (if the
	 * user specified one with -g) or looking for the largest unused
	 * value.
	 */

#ifdef NO_GETGRENT
	gr_rewind ();
	while ((grp = gr_next ())) {
#else
	setgrent ();
	while ((grp = getgrent ())) {
#endif
		if (strcmp (group_name, grp->gr_name) == 0) {
			if (fflg) {
				fail_exit (E_SUCCESS);
			}
			fprintf (stderr, _("%s: name %s is not unique\n"),
				 Prog, group_name);
			fail_exit (E_NAME_IN_USE);
		}
		if (gflg && group_id == grp->gr_gid) {
			if (fflg) {
				/* turn off -g and search again */
				gflg = 0;
#ifdef NO_GETGRENT
				gr_rewind ();
#else
				setgrent ();
#endif
				continue;
			}
			fprintf (stderr, _("%s: GID %u is not unique\n"),
				 Prog, (unsigned int) group_id);
			fail_exit (E_GID_IN_USE);
		}
		if (!gflg && grp->gr_gid >= group_id) {
			if (grp->gr_gid > gid_max)
				continue;
			group_id = grp->gr_gid + 1;
		}
	}
	if (!gflg && group_id == gid_max + 1) {
		for (group_id = gid_min; group_id < gid_max; group_id++) {
#ifdef NO_GETGRENT
			gr_rewind ();
			while ((grp = gr_next ())
			       && grp->gr_gid != group_id);
			if (!grp)
				break;
#else
			if (!getgrgid (group_id))
				break;
#endif
		}
		if (group_id == gid_max) {
			fprintf (stderr, _("%s: can't get unique GID\n"), Prog);
			fail_exit (E_GID_IN_USE);
		}
	}
}

/*
 * check_new_name - check the new name for validity
 *
 *	check_new_name() insures that the new name doesn't contain any
 *	illegal characters.
 */

static void check_new_name (void)
{
	if (check_group_name (group_name))
		return;

	/*
	 * All invalid group names land here.
	 */

	fprintf (stderr, _("%s: %s is not a valid group name\n"),
		 Prog, group_name);

	exit (E_BAD_ARG);
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this new
 *	group. This causes any modified entries to be written out.
 */

static void close_files (void)
{
	if (!gr_close ()) {
		fprintf (stderr, _("%s: cannot rewrite group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_close ()) {
		fprintf (stderr,
			 _("%s: cannot rewrite shadow group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp)
		sgr_unlock ();
#endif				/* SHADOWGRP */
}

/*
 * open_files - lock and open the group files
 *
 *	open_files() opens the two group files.
 */

static void open_files (void)
{
	if (!gr_lock ()) {
		fprintf (stderr, _("%s: unable to lock group file\n"), Prog);
		exit (E_GRP_UPDATE);
	}
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: unable to open group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_lock ()) {
		fprintf (stderr,
			 _("%s: unable to lock shadow group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp && !sgr_open (O_RDWR)) {
		fprintf (stderr,
			 _("%s: unable to open shadow group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
}

/*
 * fail_exit - exit with an error code after unlocking files
 */

static void fail_exit (int code)
{
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp)
		sgr_unlock ();
#endif
	exit (code);
}

#ifdef USE_PAM
static struct pam_conv conv = {
	misc_conv,
	NULL
};
#endif				/* USE_PAM */

/*
 * main - groupadd command
 */

int main (int argc, char **argv)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	struct passwd *pampw;
	int retval;
#endif

	/*
	 * Get my name so that I can use it to report errors.
	 */

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	OPENLOG ("groupadd");

	{
		/*
		 * Parse the command line options.
		 */
		char *cp;
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"force", no_argument, NULL, 'f'},
			{"gid", required_argument, NULL, 'g'},
			{"help", no_argument, NULL, 'h'},
			{"key", required_argument, NULL, 'K'},
			{"non-unique", required_argument, NULL, 'o'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "fg:hK:o", long_options,
				     &option_index)) != -1) {
			switch (c) {
			case 'f':
				/*
				 * "force" - do nothing, just exit(0), if the
				 * specified group already exists. With -g, if
				 * specified gid already exists, choose another
				 * (unique) gid (turn off -g). Based on the RedHat's
				 * patch from shadow-utils-970616-9.
				 */
				fflg++;
				break;
			case 'g':
				gflg++;
				if (!isdigit (optarg[0]))
					usage ();

				group_id = strtoul (optarg, &cp, 10);
				if (*cp != '\0') {
					fprintf (stderr,
						 _("%s: invalid group %s\n"),
						 Prog, optarg);
					fail_exit (E_BAD_ARG);
				}
				break;
			case 'h':
				usage ();
				break;
			case 'K':
				/*
				 * override login.defs defaults (-K name=value)
				 * example: -K GID_MIN=100 -K GID_MAX=499
				 * note: -K GID_MIN=10,GID_MAX=499 doesn't work yet
				 */
				cp = strchr (optarg, '=');
				if (!cp) {
					fprintf (stderr,
						 _
						 ("%s: -K requires KEY=VALUE\n"),
						 Prog);
					exit (E_BAD_ARG);
				}
				/* terminate name, point to value */
				*cp++ = '\0';
				if (putdef_str (optarg, cp) < 0)
					exit (E_BAD_ARG);
				break;
			case 'o':
				oflg++;
				break;
			default:
				usage ();
			}
		}
	}

	if (oflg && !gflg)
		usage ();

	if (optind != argc - 1)
		usage ();

	group_name = argv[argc - 1];
	check_new_name ();

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	pampw = getpwuid (getuid ());
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("groupadd", pampw->pw_name, &conv, &pamh);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (1);
	}
#endif				/* USE_PAM */

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	/*
	 * Start with a quick check to see if the group exists.
	 */

	if (getgrnam (group_name)) {
		if (fflg) {
			exit (E_SUCCESS);
		}
		fprintf (stderr, _("%s: group %s exists\n"), Prog, group_name);
		exit (E_NAME_IN_USE);
	}

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	if (!gflg || !oflg)
		find_new_gid ();

	grp_update ();
	nscd_flush_cache ("group");

	close_files ();

#ifdef USE_PAM
	if (retval == PAM_SUCCESS) {
		retval = pam_chauthtok (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM chauthtok failed\n"), Prog);
		exit (1);
	}

	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	exit (E_SUCCESS);
	/* NOT REACHED */
}
