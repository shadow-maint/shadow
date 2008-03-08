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
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
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
static int errors;
static gid_t group_id = -1;

#ifdef	SHADOWGRP
static int is_shadow_grp;
#endif

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
 * grp_update - update group file entries
 *
 *	grp_update() writes the new records to the group files.
 */
static void grp_update (void)
{
	if (!gr_remove (group_name)) {
		fprintf (stderr, _("%s: error removing group entry\n"), Prog);
		errors++;
	}
#ifdef	SHADOWGRP
	/*
	 * Delete the shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_locate (group_name) != NULL)) {
		if (!sgr_remove (group_name)) {
			fprintf (stderr,
			         _("%s: error removing shadow group entry\n"),
			         Prog);
			errors++;
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
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting group", group_name,
	              group_id, 1);
#endif
	SYSLOG ((LOG_INFO, "remove group `%s'\n", group_name));

	if (!gr_close ()) {
		fprintf (stderr, _("%s: cannot rewrite group file\n"), Prog);
		errors++;
	}
	gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_close ()) {
		fprintf (stderr,
			 _("%s: cannot rewrite shadow group file\n"), Prog);
		errors++;
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

	while ((pwd = getpwent ()) && pwd->pw_gid != gid);

	endpwent ();

	/*
	 * If pwd isn't NULL, it stopped because the gid's matched.
	 */

	if (pwd == (struct passwd *) 0)
		return;

	/*
	 * Can't remove the group.
	 */
	fprintf (stderr, _("%s: cannot remove user's primary group.\n"), Prog);
	exit (E_GROUP_BUSY);
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

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (argc != 2)
		usage ();

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

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("groupdel", pampw->pw_name,
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
				      "deleting group",
				      group_name, -1, 0);
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

		fprintf (stderr, _("%s: group %s is a NIS group\n"),
			 Prog, group_name);

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting group",
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

	/*
	 * Now check to insure that this isn't the primary group of
	 * anyone.
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
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	if (errors != 0)
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "deleting group",
			      group_name, -1, 0);
#endif
	exit (errors == 0 ? E_SUCCESS : E_GRP_UPDATE);
	/* NOT REACHED */
	return 0;
}
