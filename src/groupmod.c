/*
 * Copyright 1991 - 1994, Julianne Frances Haugh
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
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
/*
 * exit status values
 */
#define E_SUCCESS	0	/* success */
#define E_USAGE		2	/* invalid command syntax */
#define E_BAD_ARG	3	/* invalid argument to option */
#define E_GID_IN_USE	4	/* gid already in use (and no -o) */
#define E_NOTFOUND	6	/* specified group doesn't exist */
#define E_NAME_IN_USE	9	/* group name already in use */
#define E_GRP_UPDATE	10	/* can't update group file */
/*
 * Global variables
 */
#ifdef	SHADOWGRP
static int is_shadow_grp;
#endif
static char *group_name;
static char *group_newname;
static char *group_passwd;
static gid_t group_id;
static gid_t group_newid;

static char *Prog;

static int
    oflg = 0,			/* permit non-unique group ID to be specified with -g */
    gflg = 0,			/* new ID value for the group */
    nflg = 0,			/* a new name has been specified for the group */
    pflg = 0;			/* new encrypted password */

/* local function prototypes */
static void usage (void);
static void new_grent (struct group *);

#ifdef SHADOWGRP
static void new_sgent (struct sgrp *);
#endif
static void grp_update (void);
static void check_new_gid (void);
static void check_new_name (void);
static void process_flags (int, char **);
static void close_files (void);
static void open_files (void);
static gid_t get_gid (const char *gidstr);

/*
 * usage - display usage message and exit
 */

static void usage (void)
{
	fprintf (stderr, _("Usage: groupmod [options] GROUP\n"
			   "\n"
			   "Options:\n"
			   "  -g, --gid GID                 force use new GID by GROUP\n"
			   "  -h, --help                    display this help message and exit\n"
			   "  -n, --new-name NEW_GROUP      force use NEW_GROUP name by GROUP\n"
			   "  -o, --non-unique              allow using duplicate (non-unique) GID by GROUP\n"
			   "  -p, --password PASSWORD       use encrypted password for the new password\n"
			   "\n"));
	exit (E_USAGE);
}

/*
 * new_grent - updates the values in a group file entry
 *
 *	new_grent() takes all of the values that have been entered and fills
 *	in a (struct group) with them.
 */
static void new_grent (struct group *grent)
{
	if (nflg)
		grent->gr_name = xstrdup (group_newname);

	if (gflg)
		grent->gr_gid = group_newid;

	if (pflg)
		grent->gr_passwd = group_passwd;
}

#ifdef	SHADOWGRP
/*
 * new_sgent - updates the values in a shadow group file entry
 *
 *	new_sgent() takes all of the values that have been entered and fills
 *	in a (struct sgrp) with them.
 */
static void new_sgent (struct sgrp *sgent)
{
	if (nflg)
		sgent->sg_name = xstrdup (group_newname);

	if (pflg)
		sgent->sg_passwd = group_passwd;
}
#endif				/* SHADOWGRP */

/*
 * grp_update - update group file entries
 *
 *	grp_update() writes the new records to the group files.
 */
static void grp_update (void)
{
	struct group grp;
	const struct group *ogrp;

#ifdef	SHADOWGRP
	struct sgrp sgrp;
	const struct sgrp *osgrp = NULL;
#endif				/* SHADOWGRP */

	/*
	 * Get the current settings for this group.
	 */
	ogrp = gr_locate (group_name);
	if (!ogrp) {
		fprintf (stderr,
			 _("%s: %s not found in /etc/group\n"),
			 Prog, group_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifying group",
			      group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
	grp = *ogrp;
	new_grent (&grp);
#ifdef	SHADOWGRP
	if (is_shadow_grp && (osgrp = sgr_locate (group_name))) {
		sgrp = *osgrp;
		new_sgent (&sgrp);
		if (pflg)
			grp.gr_passwd = SHADOW_PASSWD_STRING;
	}
#endif				/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */
	if (!gr_update (&grp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding group",
			      group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
	if (nflg && !gr_remove (group_name)) {
		fprintf (stderr, _("%s: error removing group entry\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting group",
			      group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP

	/*
	 * Make sure there was a shadow entry to begin with. Skip down to
	 * "out" if there wasn't. Can't just return because there might be
	 * some syslogging to do.
	 */
	if (!osgrp)
		goto out;

	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && !sgr_update (&sgrp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding group",
			      group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp && nflg && !sgr_remove (group_name)) {
		fprintf (stderr, _("%s: error removing group entry\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting group",
			      group_name, -1, 0);
#endif
		exit (E_GRP_UPDATE);
	}
      out:
#endif				/* SHADOWGRP */

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifing group", group_name,
		      group_id, 1);
#endif
	if (nflg)
		SYSLOG ((LOG_INFO, "change group `%s' to `%s'",
			 group_name, group_newname));

	if (gflg)
		SYSLOG ((LOG_INFO, "change GID for `%s' to %u",
			 nflg ? group_newname : group_name, group_newid));
}

/*
 * check_new_gid - check the new GID value for uniqueness
 *
 *	check_new_gid() insures that the new GID value is unique.
 */
static void check_new_gid (void)
{
	/*
	 * First, the easy stuff. If the ID can be duplicated, or if the ID
	 * didn't really change, just return. If the ID didn't change, turn
	 * off those flags. No sense doing needless work.
	 */
	if (group_id == group_newid) {
		gflg = 0;
		return;
	}

	if (oflg || !getgrgid (group_newid)) /* local, no need for xgetgrgid */
		return;

	/*
	 * Tell the user what they did wrong.
	 */
	fprintf (stderr, _("%s: %u is not a unique GID\n"), Prog, group_newid);
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modify gid", NULL,
		      group_newid, 0);
#endif
	exit (E_GID_IN_USE);
}

/*
 * check_new_name - check the new name for uniqueness
 *
 *	check_new_name() insures that the new name does not exist already.
 *	You can't have the same name twice, period.
 */
static void check_new_name (void)
{
	/*
	 * Make sure they are actually changing the name.
	 */
	if (strcmp (group_name, group_newname) == 0) {
		nflg = 0;
		return;
	}

	if (check_group_name (group_newname)) {

		/*
		 * If the entry is found, too bad.
		 */
		/* local, no need for xgetgrnam */
		if (getgrnam (group_newname)) {
			fprintf (stderr,
				 _("%s: %s is not a unique name\n"), Prog,
				 group_newname);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "modifying group", group_name, -1, 0);
#endif
			exit (E_NAME_IN_USE);
		}
		return;
	}

	/*
	 * All invalid group names land here.
	 */

	fprintf (stderr, _("%s: %s is not a valid group name\n"),
		 Prog, group_newname);
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifying group", group_name,
		      -1, 0);
#endif
	exit (E_BAD_ARG);
}

/*
 * get_id - validate and get group ID
 */
static gid_t get_gid (const char *gidstr)
{
	long val;
	char *errptr;

	val = strtol (gidstr, &errptr, 10);
	if (*errptr || errno == ERANGE || val < 0) {
		fprintf (stderr, _("%s: invalid numeric argument '%s'\n"), Prog,
			 gidstr);
		exit (E_BAD_ARG);
	}
	return val;
}

/*
 * process_flags - perform command line argument setting
 *
 *	process_flags() interprets the command line arguments and sets the
 *	values that the user will be created with accordingly. The values
 *	are checked for sanity.
 */
static void process_flags (int argc, char **argv)
{

	{
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"gid", required_argument, NULL, 'g'},
			{"help", no_argument, NULL, 'h'},
			{"new-name", required_argument, NULL, 'n'},
			{"non-unique", no_argument, NULL, 'o'},
			{"password", required_argument, NULL, 'p'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c =
			getopt_long (argc, argv, "g:hn:o",
				     long_options, &option_index)) != -1) {
			switch (c) {
			case 'g':
				gflg++;
				group_newid = get_gid (optarg);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK,
					      Prog, "modifying group",
					      NULL, group_newid, 0);
#endif
				break;
			case 'n':
				nflg++;
				group_newname = optarg;
				break;
			case 'o':
				oflg++;
				break;
			case 'p':
				group_passwd = optarg;
				pflg++;
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
		exit (E_GRP_UPDATE);
	}
	gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_close ()) {
		fprintf (stderr,
			 _("%s: cannot rewrite shadow group file\n"), Prog);
		exit (E_GRP_UPDATE);
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
		exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_lock ()) {
		fprintf (stderr,
			 _("%s: unable to lock shadow group file\n"), Prog);
		exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp && !sgr_open (O_RDWR)) {
		fprintf (stderr,
			 _("%s: unable to open shadow group file\n"), Prog);
		exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
}

/*
 * main - groupmod command
 *
 *	The syntax of the groupmod command is
 *	
 *	groupmod [ -g gid [ -o ]] [ -n name ] group
 *
 *	The flags are
 *		-g - specify a new group ID value
 *		-o - permit the group ID value to be non-unique
 *		-n - specify a new group name
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

	process_flags (argc, argv);

	OPENLOG ("groupmod");

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("groupmod", pampw->pw_name,
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
	{
		struct group *grp;
		/*
		 * Start with a quick check to see if the group exists.
		 */
		/* local, no need for xgetgrnam */
		if (!(grp = getgrnam (group_name))) {
			fprintf (stderr, _("%s: group %s does not exist\n"),
				 Prog, group_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "modifying group", group_name, -1, 0);
#endif
			exit (E_NOTFOUND);
		} else
			group_id = grp->gr_gid;
	}

#ifdef WITH_AUDIT
	/* Set new name/id to original if not specified on command line */
	if (nflg == 0)
		group_newname = group_name;
	if (gflg == 0)
		group_newid = group_id;
#endif

#ifdef	USE_NIS
	/*
	 * Now make sure it isn't an NIS group.
	 */
	if (__isgrNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr, _("%s: group %s is a NIS group\n"),
			 Prog, group_name);

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifying group",
			      group_name, -1, 0);
#endif
		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "group.byname", &nis_master)) {
			fprintf (stderr, _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif

	if (gflg)
		check_new_gid ();

	if (nflg)
		check_new_name ();

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	grp_update ();
	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	exit (E_SUCCESS);
	/* NOT REACHED */
}
