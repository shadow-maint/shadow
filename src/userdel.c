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

#include "rcsid.h"
RCSID (PKG_VER "$Id: userdel.c,v 1.41 2005/06/20 10:17:09 kloczek Exp $")
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif				/* USE_PAM */
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
#include "pwauth.h"
#include "nscd.h"
/*
 * exit status values
 */
#define E_SUCCESS	0
#define E_PW_UPDATE	1	/* can't update password file */
#define E_USAGE		2	/* invalid command syntax */
#define E_NOTFOUND	6	/* specified user doesn't exist */
#define E_USER_BUSY	8	/* user currently logged in */
#define E_GRP_UPDATE	10	/* can't update group file */
#define E_HOMEDIR	12	/* can't remove home directory */
static char *user_name;
static uid_t user_id;
static char *user_home;

static char *Prog;
static int fflg = 0, rflg = 0;

#include "groupio.h"
#include "pwio.h"
#include "shadowio.h"

#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif

static int is_shadow_pwd;

#ifdef SHADOWGRP
static int is_shadow_grp;
#endif

/* local function prototypes */
static void usage (void);
static void update_groups (void);
static void close_files (void);
static void fail_exit (int);
static void open_files (void);
static void update_user (void);
static void user_busy (const char *, uid_t);
static void user_cancel (const char *);

#ifdef EXTRA_CHECK_HOME_DIR
static int path_prefix (const char *, const char *);
#endif
static int is_owner (uid_t, const char *);
static void remove_mailbox (void);

/*
 * usage - display usage message and exit
 */

static void usage (void)
{
	fprintf (stderr, _("Usage: %s [-r] name\n"), Prog);
	exit (E_USAGE);
}

/*
 * update_groups - delete user from secondary group set
 *
 *	update_groups() takes the user name that was given and searches
 *	the group files for membership in any group.
 *
 *	we also check to see if they have any groups they own (the same
 *	name is their user name) and delete them too (only if USERGROUPS_ENAB
 *	is enabled).
 */

static void update_groups (void)
{
	const struct group *grp;
	struct group *ngrp;

#ifdef	SHADOWGRP
	int deleted_user_group = 0;
	const struct sgrp *sgrp;
	struct sgrp *nsgrp;
#endif				/* SHADOWGRP */

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */

	for (gr_rewind (), grp = gr_next (); grp; grp = gr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */

		if (!is_on_list (grp->gr_mem, user_name))
			continue;

		/* 
		 * Delete the username from the list of group members and
		 * update the group entry to reflect the change.
		 */

		ngrp = __gr_dup (grp);
		if (!ngrp) {
			exit (13);	/* XXX */
		}
		ngrp->gr_mem = del_list (ngrp->gr_mem, user_name);
		if (!gr_update (ngrp))
			fprintf (stderr,
				 _("%s: error updating group entry\n"), Prog);

		/*
		 * Update the DBM group file with the new entry as well.
		 */

		SYSLOG ((LOG_INFO, "delete `%s' from group `%s'\n",
			 user_name, ngrp->gr_name));
	}
	/*
	 * we've removed their name from all the groups above, so
	 * now if they have a group with the same name as their
	 * user name, with no members, we delete it.
	 */

	grp = getgrnam (user_name);
	if (grp && getdef_bool ("USERGROUPS_ENAB")
	    && (grp->gr_mem[0] == NULL)) {

		gr_remove (grp->gr_name);

#ifdef SHADOWGRP
		deleted_user_group = 1;
#endif

		SYSLOG ((LOG_INFO, "removed group `%s' owned by `%s'\n",
			 grp->gr_name, user_name));
	}
#ifdef	SHADOWGRP
	if (!is_shadow_grp)
		return;

	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of. Both the administrative list and
	 * the ordinary membership list is checked.
	 */

	for (sgr_rewind (), sgrp = sgr_next (); sgrp; sgrp = sgr_next ()) {
		int was_member, was_admin;

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */

		was_member = is_on_list (sgrp->sg_mem, user_name);
		was_admin = is_on_list (sgrp->sg_adm, user_name);

		if (!was_member && !was_admin)
			continue;

		nsgrp = __sgr_dup (sgrp);
		if (!nsgrp) {
			exit (13);	/* XXX */
		}

		if (was_member)
			nsgrp->sg_mem = del_list (nsgrp->sg_mem, user_name);

		if (was_admin)
			nsgrp->sg_adm = del_list (nsgrp->sg_adm, user_name);

		if (!sgr_update (nsgrp))
			fprintf (stderr,
				 _("%s: error updating group entry\n"), Prog);
		SYSLOG ((LOG_INFO, "delete `%s' from shadow group `%s'\n",
			 user_name, nsgrp->sg_name));
	}

	if (deleted_user_group)
		sgr_remove (user_name);
#endif				/* SHADOWGRP */
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new user. This causes any modified entries to be written out.
 */

static void close_files (void)
{
	if (!pw_close ())
		fprintf (stderr, _("%s: cannot rewrite password file\n"), Prog);
	if (is_shadow_pwd && !spw_close ())
		fprintf (stderr,
			 _("%s: cannot rewrite shadow password file\n"), Prog);
	if (!gr_close ())
		fprintf (stderr, _("%s: cannot rewrite group file\n"), Prog);

	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_close ())
		fprintf (stderr,
			 _("%s: cannot rewrite shadow group file\n"), Prog);

	if (is_shadow_grp)
		(void) sgr_unlock ();
#endif
	if (is_shadow_pwd)
		(void) spw_unlock ();
	(void) pw_unlock ();
}

/*
 * fail_exit - exit with a failure code after unlocking the files
 */

static void fail_exit (int code)
{
	(void) pw_unlock ();
	(void) gr_unlock ();
	if (is_shadow_pwd)
		spw_unlock ();
#ifdef	SHADOWGRP
	if (is_shadow_grp)
		sgr_unlock ();
#endif
	exit (code);
}

/*
 * open_files - lock and open the password files
 *
 *	open_files() opens the two password files.
 */

static void open_files (void)
{
	if (!pw_lock ()) {
		fprintf (stderr, _("%s: unable to lock password file\n"), Prog);
		exit (E_PW_UPDATE);
	}
	if (!pw_open (O_RDWR)) {
		fprintf (stderr, _("%s: unable to open password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && !spw_lock ()) {
		fprintf (stderr,
			 _("%s: cannot lock shadow password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && !spw_open (O_RDWR)) {
		fprintf (stderr,
			 _("%s: cannot open shadow password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	if (!gr_lock ()) {
		fprintf (stderr, _("%s: unable to lock group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: cannot open group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp && !sgr_lock ()) {
		fprintf (stderr,
			 _("%s: unable to lock shadow group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	if (is_shadow_grp && !sgr_open (O_RDWR)) {
		fprintf (stderr, _("%s: cannot open shadow group file\n"),
			 Prog);
		fail_exit (E_GRP_UPDATE);
	}
#endif
}

/*
 * update_user - delete the user entries
 *
 *	update_user() deletes the password file entries for this user
 *	and will update the group entries as required.
 */

static void update_user (void)
{
	if (!pw_remove (user_name))
		fprintf (stderr,
			 _("%s: error deleting password entry\n"), Prog);
	if (is_shadow_pwd && !spw_remove (user_name))
		fprintf (stderr,
			 _("%s: error deleting shadow password entry\n"), Prog);
	SYSLOG ((LOG_INFO, "delete user `%s'\n", user_name));
}

/*
 * user_busy - see if user is logged in.
 *
 * XXX - should probably check if there are any processes owned
 * by this user. Also, I think this check should be in usermod
 * as well (at least when changing username or UID).  --marekm
 */

static void user_busy (const char *name, uid_t uid)
{

/*
 * We see if the user is logged in by looking for the user name
 * in the utmp file.
 */
#if HAVE_UTMPX_H
	struct utmpx *utent;

	setutxent ();
	while ((utent = getutxent ())) {
#else
	struct utmp *utent;

	setutent ();
	while ((utent = getutent ())) {
#endif
#ifdef USER_PROCESS
		if (utent->ut_type != USER_PROCESS)
			continue;
#else
		if (utent->ut_user[0] == '\0')
			continue;
#endif
		if (strncmp (utent->ut_user, name, sizeof utent->ut_user))
			continue;
		fprintf (stderr,
			 _("%s: user %s is currently logged in\n"), Prog, name);
		exit (E_USER_BUSY);
	}
}

/* 
 * user_cancel - cancel cron and at jobs
 *
 *	user_cancel removes the crontab and any at jobs for a user
 */

/*
 * We used to have all this stuff hardcoded here, but now
 * we just run an external script - it may need to do other
 * things as well (like removing print jobs) and we may not
 * want to recompile userdel too often. Below is a sample
 * script (should work at least on Debian 1.1).  --marekm
==========
#! /bin/sh

# Check for the required argument.
if [ $# != 1 ]; then
	echo Usage: $0 username
	exit 1
fi

# Remove cron jobs.
crontab -r -u $1

# Remove at jobs. XXX - will remove any jobs owned by the same UID, even if
# it was shared by a different username. at really should store the username
# somewhere, and atrm should support an option to remove all jobs owned by
# the specified user - for now we have to do this ugly hack...
find /var/spool/cron/atjobs -name "[^.]*" -type f -user $1 -exec rm {} \;

# Remove print jobs.
lprm $1

# All done.
exit 0
==========
 */

static void user_cancel (const char *user)
{
	char *cmd;
	int pid, wpid;
	int status;

	if (!(cmd = getdef_str ("USERDEL_CMD")))
		return;
	pid = fork ();
	if (pid == 0) {
		execl (cmd, cmd, user, (char *) 0);
		if (errno == ENOENT) {
			perror (cmd);
			_exit (127);
		} else {
			perror (cmd);
			_exit (126);
		}
	} else if (pid == -1) {
		perror ("fork");
		return;
	}
	do {
		wpid = wait (&status);
	} while (wpid != pid && wpid != -1);
}

#ifdef EXTRA_CHECK_HOME_DIR
static int path_prefix (const char *s1, const char *s2)
{
	return (strncmp (s2, s1, strlen (s1)) == 0);
}
#endif

static int is_owner (uid_t uid, const char *path)
{
	struct stat st;

	if (stat (path, &st))
		return -1;
	return (st.st_uid == uid);
}

static void remove_mailbox (void)
{
	const char *maildir;
	char mailfile[1024];
	int i;

	maildir = getdef_str ("MAIL_DIR");
#ifdef MAIL_SPOOL_DIR
	if (!maildir && !getdef_str ("MAIL_FILE"))
		maildir = MAIL_SPOOL_DIR;
#endif
	if (!maildir)
		return;
	snprintf (mailfile, sizeof mailfile, "%s/%s", maildir, user_name);
	if (fflg) {
		unlink (mailfile);	/* always remove, ignore errors */
		return;
	}
	i = is_owner (user_id, mailfile);
	if (i == 0) {
		fprintf (stderr,
			 _
			 ("%s: %s not owned by %s, not removing\n"),
			 Prog, mailfile, user_name);
		return;
	} else if (i == -1)
		return;		/* mailbox doesn't exist */
	if (unlink (mailfile)) {
		fprintf (stderr, _("%s: warning: can't remove "), Prog);
		perror (mailfile);
	}
}

#ifdef USE_PAM
static struct pam_conv conv = {
	misc_conv, NULL
};
#endif				/* USE_PAM */
/*
 * main - userdel command
 */
int main (int argc, char **argv)
{
	struct passwd *pwd;
	int arg;
	int errors = 0;

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

	while ((arg = getopt (argc, argv, "fr")) != EOF) {
		switch (arg) {
		case 'f':	/* force remove even if not owned by user */
			fflg++;
			break;
		case 'r':	/* remove home dir and mailbox */
			rflg++;
			break;
		default:
			usage ();
		}
	}

	if (optind + 1 != argc)
		usage ();

#ifdef USE_PAM
	retval = PAM_SUCCESS;
	pampw = getpwuid (getuid ());
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS)
		retval = pam_start ("userdel", pampw->pw_name, &conv, &pamh);

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS)
			pam_end (pamh, retval);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS)
			pam_end (pamh, retval);
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (E_PW_UPDATE);
	}
#endif				/* USE_PAM */

	OPENLOG ("userdel");

	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	/*
	 * Start with a quick check to see if the user exists.
	 */
	user_name = argv[argc - 1];
	if (!(pwd = getpwnam (user_name))) {
		fprintf (stderr, _("%s: user %s does not exist\n"),
			 Prog, user_name);
		exit (E_NOTFOUND);
	}
#ifdef	USE_NIS

	/*
	 * Now make sure it isn't an NIS user.
	 */

	if (__ispwNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr,
			 _("%s: user %s is a NIS user\n"), Prog, user_name);
		if (!yp_get_default_domain (&nis_domain)
		    && !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr,
				 _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif
	user_id = pwd->pw_uid;
	user_home = xstrdup (pwd->pw_dir);
	/*
	 * Check to make certain the user isn't logged in.
	 */
	user_busy (user_name, user_id);
	/*
	 * Do the hard stuff - open the files, create the user entries,
	 * create the home directory, then close and update the files.
	 */
	open_files ();
	update_user ();
	update_groups ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");
	nscd_flush_cache ("shadow");

	if (rflg)
		remove_mailbox ();
	if (rflg && !fflg && !is_owner (user_id, user_home)) {
		fprintf (stderr,
			 _("%s: %s not owned by %s, not removing\n"),
			 Prog, user_home, user_name);
		rflg = 0;
		errors++;
	}

/* This may be slow, the above should be good enough. */
#ifdef EXTRA_CHECK_HOME_DIR
	if (rflg && !fflg) {
		/*
		 * For safety, refuse to remove the home directory if it
		 * would result in removing some other user's home
		 * directory. Still not perfect so be careful, but should
		 * prevent accidents if someone has /home or / as home
		 * directory...  --marekm
		 */
		setpwent ();
		while ((pwd = getpwent ())) {
			if (strcmp (pwd->pw_name, user_name) == 0)
				continue;
			if (path_prefix (user_home, pwd->pw_dir)) {
				fprintf (stderr,
					 _
					 ("%s: not removing directory %s (would remove home of user %s)\n"),
					 Prog, user_home, pwd->pw_name);
				rflg = 0;
				errors++;
				break;
			}
		}
	}
#endif

	if (rflg) {
		if (remove_tree (user_home)
		    || rmdir (user_home)) {
			fprintf (stderr,
				 _("%s: error removing directory %s\n"),
				 Prog, user_home);
			errors++;
		}
	}

	/*
	 * Cancel any crontabs or at jobs. Have to do this before we remove
	 * the entry from /etc/passwd.
	 */

	user_cancel (user_name);
	close_files ();
#ifdef USE_PAM
	if (retval == PAM_SUCCESS) {
		retval = pam_chauthtok (pamh, 0);
		if (retval != PAM_SUCCESS)
			pam_end (pamh, retval);
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM chauthtok failed\n"), Prog);
		exit (E_PW_UPDATE);
	}

	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	exit (errors ? E_HOMEDIR : E_SUCCESS);
	/* NOT REACHED */
}
