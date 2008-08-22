/*
 * Copyright (c) 1991 - 1993, Julianne Frances Haugh
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
static char *group_passwd;
static char *empty_list = NULL;

static char *Prog;

static bool oflg = false;	/* permit non-unique group ID to be specified with -g */
static bool gflg = false;	/* ID value for the new group */
static bool fflg = false;	/* if group already exists, do nothing and exit(0) */
static bool rflg = false;	/* create a system account */
static bool pflg = false;	/* new encrypted password */

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool gr_locked = false;


#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
#endif

/* local function prototypes */
static void usage (void);
static void new_grent (struct group *grent);

#ifdef SHADOWGRP
static void new_sgent (struct sgrp *sgent);
#endif
static void grp_update (void);
static void check_new_name (void);
static void close_files (void);
static void open_files (void);
static void fail_exit (int code);
static gid_t get_gid (const char *gidstr);
static void process_flags (int argc, char **argv);
static void check_flags (void);
static void check_perms (void);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fputs (_("Usage: groupadd [options] GROUP\n"
	         "\n"
	         "Options:\n"
	         "  -f, --force                   force exit with success status if the\n"
	         "                                specified group already exists\n"
	         "  -g, --gid GID                 use GID for the new group\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -K, --key KEY=VALUE           overrides /etc/login.defs defaults\n"
	         "  -o, --non-unique              allow create group with duplicate\n"
	         "                                (non-unique) GID\n"
	         "  -p, --password PASSWORD       use encrypted password for the new group\n"
	         "  -r, --system                  create a system account\n"
	         "\n"), stderr);
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
	if (pflg) {
		grent->gr_passwd = group_passwd;
	} else {
		grent->gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
	}
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
	if (pflg) {
		sgent->sg_passwd = group_passwd;
	} else {
		sgent->sg_passwd = "!";	/* XXX warning: const */
	}
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
	if (is_shadow_grp && pflg) {
		grp.gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
	}
#endif				/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */
	if (gr_update (&grp) == 0) {
		fprintf (stderr, _("%s: error adding new entry '%s' in %s\n"), Prog, grp.gr_name, gr_dbname ());
		SYSLOG ((LOG_WARN, "error adding new entry '%s' in %s", grp.gr_name, gr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_update (&sgrp) == 0)) {
		fprintf (stderr, _("%s: error adding new entry '%s' in %s\n"), Prog, sgrp.sg_name, sgr_dbname ());
		SYSLOG ((LOG_WARN, "error adding new entry '%s' in %s", sgrp.sg_name, sgr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "adding group",
	              group_name, (unsigned int) group_id, 1);
#endif
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u",
	        group_name, (unsigned int) group_id));
}

/*
 * check_new_name - check the new name for validity
 *
 *	check_new_name() insures that the new name doesn't contain any
 *	illegal characters.
 */
static void check_new_name (void)
{
	if (is_valid_group_name (group_name)) {
		return;
	}

	/*
	 * All invalid group names land here.
	 */

	fprintf (stderr, _("%s: '%s' is not a valid group name\n"),
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
	if (gr_close () == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_WARN, "failure while writing changes to %s", gr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_WARN, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "unlocking group file",
		              group_name, AUDIT_NO_ID, 0);
#endif
		/* continue */
	}
	gr_locked = false;
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "failure while writing changes to %s", sgr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
		sgr_locked = false;
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
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "locking group file",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_WARN, "cannot open %s", gr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "opening group file",
		              group_name, AUDIT_NO_ID, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "locking gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked = true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "cannot open %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "opening gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
}

/*
 * fail_exit - exit with an error code after unlocking files
 */
static void fail_exit (int code)
{
	if (gr_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_WARN, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking group file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
#ifdef	SHADOWGRP
	if (sgr_locked) {
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
#endif

#ifdef WITH_AUDIT
	if (code != E_SUCCESS) {
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "adding group",
		              group_name, AUDIT_NO_ID, 0);
	}
#endif

#ifdef USE_PAM
	if (NULL != pamh) {
		/* If there is a PAM error, fail_exit is not called.
		 * We always end the pam transaction with PAM_SUCCESS here.
		 */
		(void) pam_end (pamh, PAM_SUCCESS);
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
	return (gid_t) val;
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
		{"non-unique", no_argument, NULL, 'o'},
		{"password", required_argument, NULL, 'p'},
		{"system", no_argument, NULL, 'r'},
		{NULL, 0, NULL, '\0'}
	};

	while ((c =
		getopt_long (argc, argv, "fg:hK:op:r", long_options,
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
			fflg = true;
			break;
		case 'g':
			gflg = true;
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
				         _("%s: -K requires KEY=VALUE\n"),
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
			oflg = true;
			break;
		case 'p':
			pflg = true;
			group_passwd = optarg;
			break;
		case 'r':
			rflg = true;
			break;
		default:
			usage ();
		}
	}

	/*
	 * Check the flags consistency
	 */
	if (optind != argc - 1) {
		usage ();
	}
	group_name = argv[optind];

	check_flags ();
}

/*
 * check_flags - check flags and parameters consistency
 *
 *	It will not return if an error is encountered.
 */
static void check_flags (void)
{
	/* -o does not make sense without -g */
	if (oflg && !gflg) {
		usage ();
	}

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
		fprintf (stderr, _("%s: group '%s' already exists\n"), Prog, group_name);
		fail_exit (E_NAME_IN_USE);
	}

	if (gflg && (getgrgid (group_id) != NULL)) {
		/* A GID was specified, and a group already exist with that GID
		 *  - either we will use this GID anyway (-o)
		 *  - either we ignore the specified GID and
		 *    we will use another one (-f)
		 *  - either it is a failure
		 */
		if (oflg) {
			/* Continue with this GID */
		} else if (fflg) {
			/* Turn off -g, we can use any GID */
			gflg = false;
		} else {
			fprintf (stderr, _("%s: GID '%lu' already exists\n"),
			         Prog, (unsigned long int) group_id);
			fail_exit (E_GID_IN_USE);
		}
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	With PAM support, the setuid bit can be set on groupadd to allow
 *	non-root users to groups.
 *	Without PAM support, only users who can write in the group databases
 *	can add groups.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms (void)
{
#ifdef USE_PAM
	int retval = PAM_SUCCESS;
	struct passwd *pampw;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("groupadd", pampw->pw_name,
		                    &conv, &pamh);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS) {
			(void) pam_end (pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS) {
			(void) pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (1);
	}
#endif				/* USE_PAM */
}

/*
 * main - groupadd command
 */
int main (int argc, char **argv)
{
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

	OPENLOG ("groupadd");

	/*
	 * Parse the command line options.
	 */
	process_flags (argc, argv);

	check_perms ();

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	/*
	 * Do the hard stuff - open the files, create the group entries,
	 * then close and update the files.
	 */
	open_files ();

	if (!gflg) {
		if (find_new_gid (rflg, &group_id, NULL) < 0) {
			fprintf (stderr, _("%s: can't create group\n"), Prog);
			fail_exit (E_GID_IN_USE);
		}
	}

	grp_update ();
	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	(void) pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	exit (E_SUCCESS);
	/* NOT REACHED */
}

