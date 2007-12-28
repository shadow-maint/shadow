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

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef USE_PAM
#include "pam_defs.h"
#include <pwd.h>
#endif				/* USE_PAM */
#include "chkname.h"
#include "defines.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
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

/*
 * Global variables
 */
static char *group_name;
static gid_t group_id;
static char *empty_list = NULL;

static char *Prog;

static int oflg = 0;		/* permit non-unique group ID to be specified with -g */
static int gflg = 0;		/* ID value for the new group */
static int fflg = 0;		/* if group already exists, do nothing and exit(0) */

/* local function prototypes */
static void usage (void);
static void new_grent (struct group *grent);

#ifdef SHADOWGRP
static void new_sgent (struct sgrp *sgent);
#endif
static void grp_update (void);
static void find_new_gid (void);
static void check_new_name (void);
static void close_files (void);
static void open_files (void);
static void fail_exit (int code);
static gid_t get_gid (const char *gidstr);
static void process_flags (int argc, char **argv);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: groupadd [options] GROUP\n"
	                   "\n"
	                   "Options:\n"
	                   "  -f, --force                   force exit with success status if the\n"
	                   "                                specified group already exists\n"
	                   "  -g, --gid GID                 use GID for the new group\n"
	                   "  -h, --help                    display this help message and exit\n"
	                   "  -K, --key KEY=VALUE           overrides /etc/login.defs defaults\n"
	                   "  -o, --non-unique              allow create group with duplicate\n"
	                   "                                (non-unique) GID\n"
	                   "\n"));
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
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding group", group_name,
	              group_id, 1);
#endif
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u",
	        group_name, (unsigned int) group_id));
}

/*
 * find_new_gid - find the next available GID
 *
 *	find_new_gid() locates the next highest unused GID in the group
 *	file.
 */
static void find_new_gid (void)
{
	const struct group *grp;
	gid_t gid_min, gid_max;

	/*
	 * It doesn't make sense to use find_new_uid(),
	 * if a GID is specified via "-g" option.
	 */
	assert (!gflg);

	gid_min = getdef_unum ("GID_MIN", 1000);
	gid_max = getdef_unum ("GID_MAX", 60000);

	/*
	 * Start with the lowest GID.
	 */
	group_id = gid_min;

	/*
	 * Search the entire group file, looking for the largest unused
	 * value.
	 */
	setgrent ();
	while ((grp = getgrent ())) {
		if ((grp->gr_gid >= group_id) && (grp->gr_gid <= gid_max)) {
			group_id = grp->gr_gid + 1;
		}
	}
	if (group_id == (gid_max + 1)) {
		for (group_id = gid_min; group_id < gid_max; group_id++) {
			/* local, no need for xgetgrgid */
			if (!getgrgid (group_id)) {
				break;
			}
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
	if (check_group_name (group_name)) {
		return;
	}

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
	if (is_shadow_grp) {
		sgr_unlock ();
	}
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
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "locking group file",
		              group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: unable to open group file\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "opening group file",
		              group_name, -1, 0);
#endif
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
	if (is_shadow_grp) {
		sgr_unlock ();
	}
#endif

#ifdef WITH_AUDIT
	if (code != E_SUCCESS) {
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding group",
		              group_name, -1, 0);
	}
#endif
	exit (code);
}

/*
 * get_id - validate and get group ID
 */
static gid_t get_gid (const char *gidstr)
{
	long val;
	char *errptr;

	val = strtol (gidstr, &errptr, 10);
	if (('\0' != *errptr) || (errno == ERANGE) || (val < 0)) {
		fprintf (stderr, _("%s: invalid numeric argument '%s'\n"),
		         Prog, gidstr);
		exit (E_BAD_ARG);
	}
	return val;
}

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
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
			group_id = get_gid (optarg);
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
			if (NULL == cp) {
				fprintf (stderr,
				         _
				         ("%s: -K requires KEY=VALUE\n"),
				         Prog);
				exit (E_BAD_ARG);
			}
			/* terminate name, point to value */
			*cp++ = '\0';
			if (putdef_str (optarg, cp) < 0) {
				exit (E_BAD_ARG);
			}
			break;
		case 'o':
			oflg++;
			break;
		default:
			usage ();
		}
	}

	/*
	 * Check the flags consistency
	 */
	if (oflg && !gflg) {
		usage ();
	}

	if (optind != argc - 1) {
		usage ();
	}

	group_name = argv[optind];

	check_new_name ();

	/*
	 * Check if the group already exist.
	 */
	/* local, no need for xgetgrnam */
	if (getgrnam (group_name) != NULL) {
		/* The group already exist */
		if (fflg) {
			/* OK, no need to do anything */
			fail_exit (E_SUCCESS);
		}
		fprintf (stderr, _("%s: group %s exists\n"), Prog, group_name);
		fail_exit (E_NAME_IN_USE);
	}

	if (gflg && (getgrgid (group_id) != NULL)) {
		/* A GID was specified, and a group already exist with that GID
		 *  - either we will use this GID anyway (-o)
		 *  - either we ignore the specified GID and
		 *    we will use another one(-f)
		 *  - either it is a failure
		 */
		if (oflg) {
			/* Continue with this GID */
		} else if (fflg) {
			/* Turn off -g, we can use any GID */
			gflg = 0;
		} else {
			fprintf (stderr, _("%s: GID %u is not unique\n"),
			         Prog, (unsigned int) group_id);
			fail_exit (E_GID_IN_USE);
		}
	}
}

/*
 * main - groupadd command
 */
int main (int argc, char **argv)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
#endif

#ifdef WITH_AUDIT
	audit_help_open ();
#endif
	/*
	 * Get my name so that I can use it to report errors.
	 */
	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	OPENLOG ("groupadd");

	/*
	 * Parse the command line options.
	 */
	process_flags (argc, argv);

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("groupadd", pampw->pw_name,
			                    &conv, &pamh);
		}
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
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	if (!gflg) {
		find_new_gid ();
	}

	grp_update ();
	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	exit (E_SUCCESS);
	/* NOT REACHED */
}

