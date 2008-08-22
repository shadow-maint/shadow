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
#include <grp.h>
#include <pwd.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
/*
 * Global variables
 */
static char *group_name;
static char *Prog;
static gid_t group_id = -1;

#ifdef	SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool gr_locked = false;

/*
 * exit status values
 */
#define E_SUCCESS	0	/* success */
#define E_USAGE		2	/* invalid command syntax */
#define E_NOTFOUND	6	/* specified group doesn't exist */
#define E_GROUP_BUSY	8	/* can't remove user's primary group */
#define E_GRP_UPDATE	10	/* can't update group file */

/* local function prototypes */
static void usage (void);
static void fail_exit (int);
static void grp_update (void);
static void close_files (void);
static void open_files (void);
static void group_busy (gid_t);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fputs (_("Usage: groupdel group\n"), stderr);
	exit (E_USAGE);
}

/*
 * fail_exit - exit with a failure code after unlocking the files
 */
static void fail_exit (int code)
{
	if (gr_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
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
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "unlocking gshadow file",
			              group_name, AUDIT_NO_ID, 0);
#endif
			/* continue */
		}
	}
#endif

#ifdef	WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "deleting group",
	              group_name, AUDIT_NO_ID, 0);
#endif

	exit (code);
}

/*
 * grp_update - update group file entries
 *
 *	grp_update() writes the new records to the group files.
 */
static void grp_update (void)
{
	if (gr_remove (group_name) == 0) {
		fprintf (stderr,
		         _("%s: cannot remove entry '%s' from %s\n"),
		         Prog, group_name, gr_dbname ());
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	/*
	 * Delete the shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_locate (group_name) != NULL)) {
		if (sgr_remove (group_name) == 0) {
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, group_name, sgr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
	return;
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new group.  This causes any modified entries to be written out.
 */
static void close_files (void)
{
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "deleting group",
	              group_name, (unsigned int) group_id, 1);
#endif
	SYSLOG ((LOG_INFO, "remove group '%s'\n", group_name));

	if (gr_close () == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", gr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
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
			         _("%s: failure while writing changes to %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", sgr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
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
		fail_exit (E_GRP_UPDATE);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_WARN, "cannot open %s", gr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked = true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_WARN, "cannot open %s", sgr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
}

/*
 * group_busy - check if this is any user's primary group
 *
 *	group_busy verifies that this group is not the primary group
 *	for any user.  You must remove all users before you remove
 *	the group.
 */
static void group_busy (gid_t gid)
{
	struct passwd *pwd;

	/*
	 * Nice slow linear search.
	 */

	setpwent ();

	while ( ((pwd = getpwent ()) != NULL) && (pwd->pw_gid != gid) );

	endpwent ();

	/*
	 * If pwd isn't NULL, it stopped because the gid's matched.
	 */

	if (pwd == (struct passwd *) 0) {
		return;
	}

	/*
	 * Can't remove the group.
	 */
	fprintf (stderr, _("%s: cannot remove the primary group of user '%s'\n"), Prog, pwd->pw_name);
	fail_exit (E_GROUP_BUSY);
}

/*
 * main - groupdel command
 *
 *	The syntax of the groupdel command is
 *	
 *	groupdel group
 *
 *	The named group will be deleted.
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

	if (argc != 2) {
		usage ();
	}

	group_name = argv[1];

	OPENLOG ("groupdel");

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (PAM_SUCCESS == retval) {
			retval = pam_start ("groupdel", pampw->pw_name,
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
		grp = getgrnam (group_name); /* local, no need for xgetgrnam */
		if (NULL == grp) {
			fprintf (stderr, _("%s: group '%s' does not exist\n"),
				 Prog, group_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "deleting group",
			              group_name, AUDIT_NO_ID, 0);
#endif
			exit (E_NOTFOUND);
		}

		group_id = grp->gr_gid;	/* LAUS */
	}

#ifdef	USE_NIS
	/*
	 * Make sure this isn't a NIS group
	 */
	if (__isgrNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr, _("%s: group '%s' is a NIS group\n"),
			 Prog, group_name);

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "deleting group",
		              group_name, AUDIT_NO_ID, 0);
#endif
		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "group.byname", &nis_master)) {
			fprintf (stderr, _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif

	/*
	 * Make sure this isn't the primary group of anyone.
	 */
	group_busy (group_id);

	/*
	 * Do the hard stuff - open the files, delete the group entries,
	 * then close and update the files.
	 */
	open_files ();

	grp_update ();

	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	(void) pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	return E_SUCCESS;
}

