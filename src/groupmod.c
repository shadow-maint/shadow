/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
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
#ifdef	SHADOWGRP
static bool is_shadow_grp;
static bool gshadow_locked = false;
#endif				/* SHADOWGRP */
static bool group_locked = false;
static bool passwd_locked = false;
static char *group_name;
static char *group_newname;
static char *group_passwd;
static gid_t group_id;
static gid_t group_newid;

static char *Prog;

static bool
    oflg = false,		/* permit non-unique group ID to be specified with -g */
    gflg = false,		/* new ID value for the group */
    nflg = false,		/* a new name has been specified for the group */
    pflg = false;		/* new encrypted password */

/* local function prototypes */
static void usage (void);
static void fail_exit (int);
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

static void fail_exit (int status)
{
	if (group_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: cannot unlock the group file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot unlock the group file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking group file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
#ifdef	SHADOWGRP
	if (gshadow_locked) {
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: cannot unlock the shadow group file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot unlock the shadow group file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
#endif				/* SHADOWGRP */
	if (passwd_locked) {
		if (pw_unlock () == 0) {
			fprintf (stderr, _("%s: cannot unlock the passwd file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot unlock the passwd file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking passwd file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
	exit (status);
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
			 _("%s: group '%s' does not exist in the group file\n"),
			 Prog, group_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "modifying group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	grp = *ogrp;
	new_grent (&grp);
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
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
		fprintf (stderr, _("%s: cannot add entry '%s' to the group database\n"), Prog, grp.gr_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "adding group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	if (nflg && (gr_remove (group_name) == 0)) {
		fprintf (stderr, _("%s: cannot remove the entry of '%s' from the group database\n"), Prog, grp.gr_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "deleting group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP

	/*
	 * Make sure there was a shadow entry to begin with. Skip down to
	 * "out" if there wasn't. Can't just return because there might be
	 * some syslogging to do.
	 */
	if (NULL == osgrp)
		goto out;

	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_update (&sgrp) == 0)) {
		fprintf (stderr, _("%s: cannot add entry '%s' to the shadow group database\n"), Prog, sgrp.sg_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "adding group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp && nflg && (sgr_remove (group_name) == 0)) {
		fprintf (stderr, _("%s: cannot remove the entry of '%s' from the shadow group database\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "deleting group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
      out:
#endif				/* SHADOWGRP */

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "modifing group",
	              group_name, (unsigned int) group_id, 1);
#endif
	if (nflg) {
		SYSLOG ((LOG_INFO, "change group '%s' to '%s'",
			 group_name, group_newname));
	}

	if (gflg) {
		SYSLOG ((LOG_INFO, "change GID for '%s' to %lu",
		          nflg ? group_newname : group_name,
		         (unsigned long) group_newid));
	}
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
	fprintf (stderr, _("%s: GID '%lu' already exists\n"),
	         Prog, (unsigned long int) group_newid);
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "modify gid",
	              NULL, (unsigned int) group_newid, 0);
#endif
	fail_exit (E_GID_IN_USE);
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
				 _("%s: group '%s' already exists\n"), Prog,
				 group_newname);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "modifying group",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_NAME_IN_USE);
		}
		return;
	}

	/*
	 * All invalid group names land here.
	 */

	fprintf (stderr, _("%s: invalid group name '%s'\n"),
		 Prog, group_newname);
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "modifying group",
	              group_name, AUDIT_NO_ID, 0);
#endif
	fail_exit (E_BAD_ARG);
}

/*
 * get_id - validate and get group ID
 */
static gid_t get_gid (const char *gidstr)
{
	long val;
	char *errptr;

	val = strtol (gidstr, &errptr, 10); /* FIXME: Should be strtoul ? */
	if (('\0' != *errptr) || (ERANGE == errno) || (val < 0)) {
		fprintf (stderr, _("%s: invalid numeric argument '%s'\n"), Prog,
			 gidstr);
		fail_exit (E_BAD_ARG);
	}
	return (gid_t) val;
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
			getopt_long (argc, argv, "g:hn:op:",
				     long_options, &option_index)) != -1) {
			switch (c) {
			case 'g':
				gflg = true;
				group_newid = get_gid (optarg);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				              "modifying group",
				              NULL, (unsigned int) group_newid, 0);
#endif
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
		fprintf (stderr, _("%s: cannot rewrite group file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot rewrite the group file"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "rewrite group file",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: cannot unlock the group file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot unlock the group file"));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "unlocking group file",
		              group_name, AUDIT_NO_ID, 0);
#endif
		/* continue */
	}
	group_locked = false;
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_close () == 0)) {
			fprintf (stderr,
			         _("%s: cannot rewrite the shadow group file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot rewrite the shadow group file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "rewrite gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: cannot unlock the shadow group file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot unlock the shadow group file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking gshadow file",
			              group, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
		gshadow_locked = false;
	}
#endif				/* SHADOWGRP */
	if (gflg) {
		if (pw_close () == 0) {
			fprintf (stderr,
			         _("%s: cannot rewrite the passwd file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot rewrite the passwd file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "rewrite passwd file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
		if (pw_unlock () == 0) {
			fprintf (stderr, _("%s: cannot unlock the passwd file\n"), Prog);
			SYSLOG ((LOG_WARN, "cannot unlock the passwd file"));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking passwd file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
		passwd_locked = false;
	}
}

/*
 * open_files - lock and open the group files
 *
 *	open_files() opens the two group files.
 */
static void open_files (void)
{
	if (gr_lock () == 0) {
		fprintf (stderr, _("%s: cannot lock the group file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot lock the group file"));
		fail_exit (E_GRP_UPDATE);
	}
	group_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open the group file\n"), Prog);
		SYSLOG ((LOG_WARN, "cannot open the group file"));
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock the shadow group file\n"),
			         Prog);
			SYSLOG ((LOG_WARN, "cannot lock the shadow group file"));
			fail_exit (E_GRP_UPDATE);
		}
		gshadow_locked = true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open the shadow group file\n"),
			         Prog);
			SYSLOG ((LOG_WARN, "cannot open the shadow group file"));
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
	if (gflg) {
		if (pw_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock the passwd file\n"),
			         Prog);
			SYSLOG ((LOG_WARN, "cannot lock the passwd file"));
			fail_exit (E_GRP_UPDATE);
		}
		passwd_locked = true;
		if (pw_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open the passwd file\n"),
			         Prog);
			SYSLOG ((LOG_WARN, "cannot open the passwd file"));
			fail_exit (E_GRP_UPDATE);
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
				         _("%s: cannot change the primary group of user '%s' from %lu to %lu, since it is not in the passwd file.\n"),
				         Prog, pwd->pw_name,
				         (unsigned long) ogid,
				         (unsigned long) ngid);
				fail_exit (E_GRP_UPDATE);
			} else {
				npwd = *lpwd;
				npwd.pw_gid = ngid;
				if (pw_update (&npwd) == 0) {
					fprintf (stderr,
					         _("%s: cannot change the primary group of user '%s' from %lu to %lu.\n"),
					         Prog, pwd->pw_name,
					         (unsigned long) ogid,
					         (unsigned long) ngid);
					fail_exit (E_GRP_UPDATE);
				}
			}
		}
	}
	endpwent ();
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

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

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

		if (PAM_SUCCESS == retval) {
			retval = pam_start ("groupmod", pampw->pw_name,
					    &conv, &pamh);
		}
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_authenticate (pamh, 0);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_acct_mgmt (pamh, 0);
	}

	if (PAM_SUCCESS != retval) {
		(void) pam_end (pamh, retval);
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		fail_exit (1);
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
		grp = getgrnam (group_name); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf (stderr, _("%s: group '%s' does not exist\n"),
				 Prog, group_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "modifying group",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_NOTFOUND);
		} else {
			group_id = grp->gr_gid;
		}
	}

#ifdef WITH_AUDIT
	/* Set new name/id to original if not specified on command line */
	if (!nflg) {
		group_newname = group_name;
	}
	if (!gflg) {
		group_newid = group_id;
	}
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
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "modifying group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "group.byname", &nis_master)) {
			fprintf (stderr, _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		fail_exit (E_NOTFOUND);
	}
#endif

	if (gflg) {
		check_new_gid ();
	}

	if (nflg) {
		check_new_name ();
	}

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	grp_update ();

	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	(void) pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	exit (E_SUCCESS);
	/* NOT REACHED */
}

