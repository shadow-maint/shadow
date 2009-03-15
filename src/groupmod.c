/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
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

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#include <pwd.h>
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
#include "chkname.h"
#include "defines.h"
#include "groupio.h"
#include "pwio.h"
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
char *Prog;

#ifdef	SHADOWGRP
static bool is_shadow_grp;
#endif				/* SHADOWGRP */
static char *group_name;
static char *group_newname;
static char *group_passwd;
static gid_t group_id;
static gid_t group_newid;

struct cleanup_info_mod info_passwd;
struct cleanup_info_mod info_group;
#ifdef	SHADOWGRP
struct cleanup_info_mod info_gshadow;
#endif

static bool
    oflg = false,		/* permit non-unique group ID to be specified with -g */
    gflg = false,		/* new ID value for the group */
    nflg = false,		/* a new name has been specified for the group */
    pflg = false;		/* new encrypted password */

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
static void lock_files (void);
static void prepare_failure_reports (void);
static void open_files (void);
static void close_files (void);
static void update_primary_groups (gid_t ogid, gid_t ngid);

/*
 * usage - display usage message and exit
 */

static void usage (void)
{
	fputs (_("Usage: groupmod [options] GROUP\n"
	         "\n"
	         "Options:\n"
	         "  -g, --gid GID                 force use new GID by GROUP\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -n, --new-name NEW_GROUP      force use NEW_GROUP name by GROUP\n"
	         "  -o, --non-unique              allow using duplicate (non-unique) GID by GROUP\n"
	         "  -p, --password PASSWORD       use encrypted password for the new password\n"
	         "\n"), stderr);
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
	if (nflg) {
		grent->gr_name = xstrdup (group_newname);
	}

	if (gflg) {
		grent->gr_gid = group_newid;
	}

	if (pflg) {
		grent->gr_passwd = group_passwd;
	}
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
	if (nflg) {
		sgent->sg_name = xstrdup (group_newname);
	}

	if (pflg) {
		sgent->sg_passwd = group_passwd;
	}
}
#endif				/* SHADOWGRP */

/*
 * grp_update - update group file entries
 *
 *	grp_update() updates the new records in the memory databases.
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
		         _("%s: group '%s' does not exist in %s\n"),
		         Prog, group_name, gr_dbname ());
		exit (E_GRP_UPDATE);
	}
	grp = *ogrp;
	new_grent (&grp);
#ifdef	SHADOWGRP
	if (   is_shadow_grp
	    && (pflg || nflg)) {
		osgrp = sgr_locate (group_name);
		if (NULL != osgrp) {
			sgrp = *osgrp;
			new_sgent (&sgrp);
			if (pflg) {
				grp.gr_passwd = SHADOW_PASSWD_STRING;
			}
		}
	}
#endif				/* SHADOWGRP */

	if (gflg) {
		update_primary_groups (ogrp->gr_gid, group_newid);
	}

	/*
	 * Write out the new group file entry.
	 */
	if (gr_update (&grp) == 0) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, gr_dbname (), grp.gr_name);
		exit (E_GRP_UPDATE);
	}
	if (nflg && (gr_remove (group_name) == 0)) {
		fprintf (stderr,
		         _("%s: cannot remove entry '%s' from %s\n"),
		         Prog, grp.gr_name, gr_dbname ());
		exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP

	/*
	 * Make sure there was a shadow entry to begin with.
	 */
	if (   (NULL != osgrp)
	    && (pflg || nflg)) {
		/*
		 * Write out the new shadow group entries as well.
		 */
		if (sgr_update (&sgrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, sgr_dbname (), sgrp.sg_name);
			exit (E_GRP_UPDATE);
		}
		if (nflg && (sgr_remove (group_name) == 0)) {
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, group_name, sgr_dbname ());
			exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
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

	if (oflg ||
	    (getgrgid (group_newid) == NULL) /* local, no need for xgetgrgid */
	   ) {
		return;
	}

	/*
	 * Tell the user what they did wrong.
	 */
	fprintf (stderr,
	         _("%s: GID '%lu' already exists\n"),
	         Prog, (unsigned long int) group_newid);
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

	if (is_valid_group_name (group_newname)) {

		/*
		 * If the entry is found, too bad.
		 */
		/* local, no need for xgetgrnam */
		if (getgrnam (group_newname) != NULL) {
			fprintf (stderr,
			         _("%s: group '%s' already exists\n"),
			         Prog, group_newname);
			exit (E_NAME_IN_USE);
		}
		return;
	}

	/*
	 * All invalid group names land here.
	 */

	fprintf (stderr,
	         _("%s: invalid group name '%s'\n"),
	         Prog, group_newname);
	exit (E_BAD_ARG);
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
		getopt_long (argc, argv, "g:hn:op:",
		             long_options, &option_index)) != -1) {
		switch (c) {
		case 'g':
			gflg = true;
			if (   (get_gid (optarg, &group_newid) == 0)
			    || (group_newid == (gid_t)-1)) {
				fprintf (stderr,
				         _("%s: invalid group ID '%s'\n"),
				         Prog, optarg);
				exit (E_BAD_ARG);
			}
			break;
		case 'n':
			nflg = true;
			group_newname = optarg;
			break;
		case 'o':
			oflg = true;
			break;
		case 'p':
			group_passwd = optarg;
			pflg = true;
			break;
		default:
			usage ();
		}
	}

	if (oflg && !gflg) {
		usage ();
	}

	if (optind != (argc - 1)) {
		usage ();
	}

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
	if (gr_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, gr_dbname ());
		exit (E_GRP_UPDATE);
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_ACCT, Prog,
	              info_group.audit_msg,
	              group_name, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
	SYSLOG ((LOG_INFO,
	         "group changed in %s (%s)",
	         gr_dbname (), info_group.action));
	del_cleanup (cleanup_report_mod_group);

	cleanup_unlock_group (NULL);
	del_cleanup (cleanup_unlock_group);

#ifdef	SHADOWGRP
	if (   is_shadow_grp
	    && (pflg || nflg)) {
		if (sgr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, sgr_dbname ());
			exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_ACCT, Prog,
		              info_gshadow.audit_msg,
		              group_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_SUCCESS);
#endif
		SYSLOG ((LOG_INFO,
		         "group changed in %s (%s)",
		         sgr_dbname (), info_gshadow.action));
		del_cleanup (cleanup_report_mod_gshadow);

		cleanup_unlock_gshadow (NULL);
		del_cleanup (cleanup_unlock_gshadow);
	}
#endif				/* SHADOWGRP */

	if (gflg) {
		if (pw_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, pw_dbname ());
			exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_ACCT, Prog,
		              info_passwd.audit_msg,
		              group_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_SUCCESS);
#endif
		SYSLOG ((LOG_INFO,
		         "group changed in %s (%s)",
		         pw_dbname (), info_passwd.action));
		del_cleanup (cleanup_report_mod_passwd);

		cleanup_unlock_passwd (NULL);
		del_cleanup (cleanup_unlock_passwd);
	}

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_ACCT, Prog,
	              "modifying group",
	              group_name, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
}

/*
 * prepare_failure_reports - Prepare the cleanup_info structure for logging
 * of success and failure to syslog or audit.
 */
static void prepare_failure_reports (void)
{
	info_group.name   = group_name;
#ifdef	SHADOWGRP
	info_gshadow.name = group_name;
#endif
	info_passwd.name  = group_name;

	info_group.audit_msg   = xmalloc (512);
#ifdef	SHADOWGRP
	info_gshadow.audit_msg = xmalloc (512);
#endif
	info_passwd.audit_msg  = xmalloc (512);

	snprintf (info_group.audit_msg, 511,
	          "changing %s; ", gr_dbname ());
#ifdef	SHADOWGRP
	snprintf (info_gshadow.audit_msg, 511,
	          "changing %s; ", sgr_dbname ());
#endif
	snprintf (info_passwd.audit_msg, 511,
	          "changing %s; ", pw_dbname ());

	info_group.action   =   info_group.audit_msg
	                      + strlen (info_group.audit_msg);
#ifdef	SHADOWGRP
	info_gshadow.action =   info_gshadow.audit_msg
	                      + strlen (info_gshadow.audit_msg);
#endif
	info_passwd.action  =   info_passwd.audit_msg
	                      + strlen (info_passwd.audit_msg);

	snprintf (info_group.action,   511 - strlen (info_group.audit_msg),
	          "group %s/%d", group_name, group_id);
#ifdef	SHADOWGRP
	snprintf (info_gshadow.action, 511 - strlen (info_group.audit_msg),
	          "group %s", group_name);
#endif
	snprintf (info_passwd.action,  511 - strlen (info_group.audit_msg),
	          "group %s/%d", group_name, group_id);

	if (nflg) {
		strncat (info_group.action, ", new name: ",
		         511 - strlen (info_group.audit_msg));
		strncat (info_group.action, group_newname,
		         511 - strlen (info_group.audit_msg));

#ifdef	SHADOWGRP
		strncat (info_gshadow.action, ", new name: ",
		         511 - strlen (info_gshadow.audit_msg));
		strncat (info_gshadow.action, group_newname,
		         511 - strlen (info_gshadow.audit_msg));
#endif

		strncat (info_passwd.action, ", new name: ",
		         511 - strlen (info_passwd.audit_msg));
		strncat (info_passwd.action, group_newname,
		         511 - strlen (info_passwd.audit_msg));
	}
	if (pflg) {
		strncat (info_group.action, ", new password",
		         511 - strlen (info_group.audit_msg));

#ifdef	SHADOWGRP
		strncat (info_gshadow.action, ", new password",
		         511 - strlen (info_gshadow.audit_msg));
#endif
	}
	if (gflg) {
		strncat (info_group.action, ", new gid: ",
		         511 - strlen (info_group.audit_msg));
		snprintf (info_group.action+strlen (info_group.action),
		          511 - strlen (info_group.audit_msg),
		          "%d", group_newid);

		strncat (info_passwd.action, ", new gid: ",
		         511 - strlen (info_passwd.audit_msg));
		snprintf (info_passwd.action+strlen (info_passwd.action),
		          511 - strlen (info_passwd.audit_msg),
		          "%d", group_newid);
	}
	info_group.audit_msg[511]   = '\0';
#ifdef	SHADOWGRP
	info_gshadow.audit_msg[511] = '\0';
#endif
	info_passwd.audit_msg[511]  = '\0';

// FIXME: add a system cleanup
	add_cleanup (cleanup_report_mod_group, &info_group);
#ifdef	SHADOWGRP
	if (   is_shadow_grp
	    && (pflg || nflg)) {
		add_cleanup (cleanup_report_mod_gshadow, &info_gshadow);
	}
#endif
	if (gflg) {
		add_cleanup (cleanup_report_mod_passwd, &info_passwd);
	}

}

/*
 * lock_files - lock the accounts databases
 *
 *	lock_files() locks the group, gshadow, and passwd databases.
 */
static void lock_files (void)
{
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
		exit (E_GRP_UPDATE);
	}
	add_cleanup (cleanup_unlock_group, NULL);

#ifdef	SHADOWGRP
	if (   is_shadow_grp
	    && (pflg || nflg)) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			exit (E_GRP_UPDATE);
		}
		add_cleanup (cleanup_unlock_gshadow, NULL);
	}
#endif

	if (gflg) {
		if (pw_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, pw_dbname ());
			exit (E_GRP_UPDATE);
		}
		add_cleanup (cleanup_unlock_passwd, NULL);
	}
}


/*
 * open_files - open the accounts databases
 *
 *	open_files() opens the group, gshadow, and passwd databases.
 */
static void open_files (void)
{
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_WARN, "cannot open %s", gr_dbname ()));
		exit (E_GRP_UPDATE);
	}

#ifdef	SHADOWGRP
	if (   is_shadow_grp
	    && (pflg || nflg)) {
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "cannot open %s", sgr_dbname ()));
			exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */

	if (gflg) {
		if (pw_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, pw_dbname ());
			SYSLOG ((LOG_WARN, "cannot open %s", gr_dbname ()));
			exit (E_GRP_UPDATE);
		}
	}
}

void update_primary_groups (gid_t ogid, gid_t ngid)
{
	struct passwd *pwd;

	setpwent ();
	while ((pwd = getpwent ()) != NULL) {
		if (pwd->pw_gid == ogid) {
			const struct passwd *lpwd;
			struct passwd npwd;
			lpwd = pw_locate (pwd->pw_name);
			if (NULL == lpwd) {
				fprintf (stderr,
				         _("%s: user '%s' does not exist in %s\n"),
				         Prog, pwd->pw_name, pw_dbname ());
				exit (E_GRP_UPDATE);
			} else {
				npwd = *lpwd;
				npwd.pw_gid = ngid;
				if (pw_update (&npwd) == 0) {
					fprintf (stderr,
					         _("%s: failed to prepare the new %s entry '%s'\n"),
					         Prog, pw_dbname (), npwd.pw_name);
					exit (E_GRP_UPDATE);
				}
			}
		}
	}
	endpwent ();
}

/*
 * main - groupmod command
 *
 */
int main (int argc, char **argv)
{
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

#ifdef WITH_AUDIT
	audit_help_open ();
#endif
	atexit (do_cleanups);

	/*
	 * Get my name so that I can use it to report errors.
	 */
	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_flags (argc, argv);

	OPENLOG ("groupmod");

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (NULL == pamh) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			exit (1);
		}

		retval = pam_start ("groupmod", pampw->pw_name, &conv, &pamh);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_authenticate (pamh, 0);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_acct_mgmt (pamh, 0);
	}

	if (NULL != pamh) {
		(void) pam_end (pamh, retval);
	}
	if (PAM_SUCCESS != retval) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (1);
	}
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif
	{
		struct group *grp;
		/*
		 * Start with a quick check to see if the group exists.
		 */
		grp = getgrnam (group_name); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf (stderr,
			         _("%s: group '%s' does not exist\n"),
			         Prog, group_name);
			exit (E_NOTFOUND);
		} else {
			group_id = grp->gr_gid;
		}
	}

#ifdef	USE_NIS
	/*
	 * Now make sure it isn't an NIS group.
	 */
	if (__isgrNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr,
		         _("%s: group %s is a NIS group\n"),
		         Prog, group_name);

		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "group.byname", &nis_master)) {
			fprintf (stderr,
			         _("%s: %s is the NIS master\n"),
			         Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif

	if (gflg) {
		check_new_gid ();
	}

	if (nflg) {
		check_new_name ();
	}

	lock_files ();

	/*
	 * Now if the group is not changed, it's our fault.
	 * Make sure failures will be reported.
	 */
	prepare_failure_reports ();

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	grp_update ();

	close_files ();

	nscd_flush_cache ("group");

	exit (E_SUCCESS);
	/* NOT REACHED */
}

