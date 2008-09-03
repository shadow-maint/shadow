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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/stat.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include "defines.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#include "shadowio.h"
#include "exitcodes.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
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
static bool fflg = false;
static bool rflg = false;

static bool is_shadow_pwd;

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool pw_locked  = false;
static bool gr_locked   = false;
static bool spw_locked  = false;

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
static bool path_prefix (const char *, const char *);
#endif
static int is_owner (uid_t, const char *);
static void remove_mailbox (void);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fputs (_("Usage: userdel [options] LOGIN\n"
	         "\n"
	         "Options:\n"
	         "  -f, --force                   force removal of files,\n"
	         "                                even if not owned by user\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -r, --remove                  remove home directory and mail spool\n"
	         "\n"), stderr);
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
	struct passwd *pwd;

#ifdef	SHADOWGRP
	bool deleted_user_group = false;
	const struct sgrp *sgrp;
	struct sgrp *nsgrp;
#endif				/* SHADOWGRP */

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */
	for (gr_rewind (), grp = gr_next (); NULL != grp; grp = gr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		if (!is_on_list (grp->gr_mem, user_name)) {
			continue;
		}

		/* 
		 * Delete the username from the list of group members and
		 * update the group entry to reflect the change.
		 */
		ngrp = __gr_dup (grp);
		if (NULL == ngrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, gr_dbname ());
			exit (13);	/* XXX */
		}
		ngrp->gr_mem = del_list (ngrp->gr_mem, user_name);
		if (gr_update (ngrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, gr_dbname (), ngrp->gr_name);
			exit (E_GRP_UPDATE);
		}

		/*
		 * Update the DBM group file with the new entry as well.
		 */
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting user from group",
		              user_name, (unsigned int) user_id, 1);
#endif
		SYSLOG ((LOG_INFO, "delete '%s' from group '%s'\n",
			 user_name, ngrp->gr_name));
	}

	/*
	 * we've removed their name from all the groups above, so
	 * now if they have a group with the same name as their
	 * user name, with no members, we delete it.
	 */
	grp = xgetgrnam (user_name);
	if (   (NULL != grp)
	    && getdef_bool ("USERGROUPS_ENAB")
	    && (NULL == grp->gr_mem[0])) {

		pwd = NULL;
		if (!fflg) {
			/*
			 * Scan the passwd file to check if this group is still
			 * used as a primary group.
			 */
			setpwent ();
			while ((pwd = getpwent ()) != NULL) {
				if (strcmp (pwd->pw_name, user_name) == 0) {
					continue;
				}
				if (pwd->pw_gid == grp->gr_gid) {
					fprintf (stderr,
					         _("%s: Cannot remove group %s which is a primary group for another user.\n"),
					         Prog, grp->gr_name);
					break;
				}
			}
			endpwent ();
		}

		if (NULL == pwd) {
			/*
			 * We can remove this group, it is not the primary
			 * group of any remaining user.
			 */
			if (gr_remove (grp->gr_name) == 0) {
				fprintf (stderr,
				         _("%s: cannot remove entry '%s' from %s\n"),
				         Prog, grp->gr_name, gr_dbname ());
				fail_exit (E_GRP_UPDATE);
			}

#ifdef SHADOWGRP
			deleted_user_group = true;
#endif

#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_GROUP, Prog,
			              "deleting group",
			              grp->gr_name, AUDIT_NO_ID, 1);
#endif
			SYSLOG ((LOG_INFO,
				 "removed group '%s' owned by '%s'\n",
				 grp->gr_name, user_name));
		}
	}
#ifdef	SHADOWGRP
	if (!is_shadow_grp) {
		return;
	}

	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of. Both the administrative list and
	 * the ordinary membership list is checked.
	 */
	for (sgr_rewind (), sgrp = sgr_next ();
	     NULL != sgrp;
	     sgrp = sgr_next ()) {
		bool was_member, was_admin;

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		was_member = is_on_list (sgrp->sg_mem, user_name);
		was_admin = is_on_list (sgrp->sg_adm, user_name);

		if (!was_member && !was_admin) {
			continue;
		}

		nsgrp = __sgr_dup (sgrp);
		if (NULL == nsgrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, sgr_dbname ());
			exit (13);	/* XXX */
		}

		if (was_member) {
			nsgrp->sg_mem = del_list (nsgrp->sg_mem, user_name);
		}

		if (was_admin) {
			nsgrp->sg_adm = del_list (nsgrp->sg_adm, user_name);
		}

		if (sgr_update (nsgrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, sgr_dbname (), nsgrp->sg_name);
			exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting user from shadow group",
		              user_name, (unsigned int) user_id, 1);
#endif
		SYSLOG ((LOG_INFO, "delete '%s' from shadow group '%s'\n",
			 user_name, nsgrp->sg_name));
	}

	if (deleted_user_group) {
		/* FIXME: Test if the group is in gshadow first? */
		if (sgr_remove (user_name) == 0) {
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, user_name, sgr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
	}
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
	if (pw_close () == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", pw_dbname ()));
		fail_exit (E_PW_UPDATE);
	}
	if (pw_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
		/* continue */
	}
	pw_locked = false;

	if (is_shadow_pwd) {
		if (spw_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", spw_dbname ()));
			fail_exit (E_PW_UPDATE);
		}
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
		spw_locked = false;
	}

	if (gr_close () == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", gr_dbname ()));
		fail_exit (E_GRP_UPDATE);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
		/* continue */
	}
	gr_locked = false;

#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", sgr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}

		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
			/* continue */
		}
		sgr_locked = false;
	}
#endif
}

/*
 * fail_exit - exit with a failure code after unlocking the files
 */
static void fail_exit (int code)
{
	if (pw_locked) {
		if (pw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
			/* continue */
		}
	}
	if (gr_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
			/* continue */
		}
	}
	if (spw_locked) {
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
	}
#ifdef	SHADOWGRP
	if (sgr_locked) {
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
			/* continue */
		}
	}
#endif

#ifdef WITH_AUDIT
	audit_logger (AUDIT_DEL_USER, Prog,
	              "deleting user",
	              user_name, (unsigned int) user_id, 0);
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
	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "locking password file",
		              user_name, (unsigned int) user_id, 0);
#endif
		fail_exit (E_PW_UPDATE);
	}
	pw_locked = true;
	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, pw_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "opening password file",
		              user_name, (unsigned int) user_id, 0);
#endif
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd) {
		if (spw_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, spw_dbname ());
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "locking shadow password file",
			              user_name, (unsigned int) user_id, 0);
#endif
			fail_exit (E_PW_UPDATE);
		}
		spw_locked = true;
		if (spw_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, spw_dbname ());
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "opening shadow password file",
			              user_name, (unsigned int) user_id, 0);
#endif
			fail_exit (E_PW_UPDATE);
		}
	}
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "locking group file",
		              user_name, (unsigned int) user_id, 0);
#endif
		fail_exit (E_GRP_UPDATE);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "opening group file",
		              user_name, (unsigned int) user_id, 0);
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
			audit_logger (AUDIT_DEL_USER, Prog,
			              "locking shadow group file",
			              user_name, (unsigned int) user_id, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked= true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr, _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "opening shadow group file",
			              user_name, (unsigned int) user_id, 0);
#endif
			fail_exit (E_GRP_UPDATE);
		}
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
	if (pw_remove (user_name) == 0) {
		fprintf (stderr,
		         _("%s: cannot remove entry '%s' from %s\n"),
		         Prog, user_name, pw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && (spw_remove (user_name) == 0)) {
		fprintf (stderr,
		         _("%s: cannot remove entry '%s' from %s\n"),
		         Prog, user_name, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_DEL_USER, Prog,
	              "deleting user entries",
	              user_name, (unsigned int) user_id, 1);
#endif
	SYSLOG ((LOG_INFO, "delete user '%s'\n", user_name));
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
	while ((utent = getutxent ()) != NULL) {
#else
	struct utmp *utent;

	setutent ();
	while ((utent = getutent ()) != NULL) {
#endif
		if (utent->ut_type != USER_PROCESS)
			continue;

		if (strncmp (utent->ut_user, name, sizeof utent->ut_user) != 0) {
			continue;
		}
		fprintf (stderr,
			 _("%s: user %s is currently logged in\n"), Prog, name);
		if (!fflg) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting user logged in",
			              name, AUDIT_NO_ID, 0);
#endif
			exit (E_USER_BUSY);
		}
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
	pid_t pid, wpid;
	int status;

	cmd = getdef_str ("USERDEL_CMD");
	if (NULL == cmd) {
		return;
	}
	pid = fork ();
	if (pid == 0) {
		execl (cmd, cmd, user, (char *) 0);
		perror (cmd);
		_exit (errno == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if ((pid_t)-1 == pid) {
		perror ("fork");
		return;
	}
	do {
		wpid = wait (&status);
	} while ((wpid != pid) && ((pid_t)-1 != wpid));
}

#ifdef EXTRA_CHECK_HOME_DIR
static bool path_prefix (const char *s1, const char *s2)
{
	return (   (strncmp (s2, s1, strlen (s1)) == 0)
	        && (   ('\0' == s2[strlen (s1)])
	            || ('/'  == s2[strlen (s1)])));
}
#endif

static int is_owner (uid_t uid, const char *path)
{
	struct stat st;

	if (stat (path, &st) != 0) {
		return -1;
	}
	return (st.st_uid == uid);
}

static void remove_mailbox (void)
{
	const char *maildir;
	char mailfile[1024];
	int i;

	maildir = getdef_str ("MAIL_DIR");
#ifdef MAIL_SPOOL_DIR
	if ((NULL == maildir) && (getdef_str ("MAIL_FILE") == NULL)) {
		maildir = MAIL_SPOOL_DIR;
	}
#endif
	if (NULL == maildir) {
		return;
	}
	snprintf (mailfile, sizeof mailfile, "%s/%s", maildir, user_name);
	if (fflg) {
		if (unlink (mailfile) != 0) {
			fprintf (stderr, _("%s: warning: can't remove %s: %s"), Prog, mailfile, strerror (errno));
			SYSLOG ((LOG_ERR, "Cannot remove %s: %s", mailfile, strerror (errno)));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting mail file",
			              user_name, (unsigned int) user_id, 0);
#endif
			/* continue */
		}
#ifdef WITH_AUDIT
		else
		{
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting mail file",
			              user_name, (unsigned int) user_id, 1);
		}
#endif
		return;
	}
	i = is_owner (user_id, mailfile);
	if (i == 0) {
		fprintf (stderr,
		         _("%s: %s not owned by %s, not removing\n"),
		         Prog, mailfile, user_name);
		SYSLOG ((LOG_ERR, "%s not owned by %s, not removed", mailfile, strerror (errno)));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id, 0);
#endif
		return;
	} else if (i == -1) {
		return;		/* mailbox doesn't exist */
	}
	if (unlink (mailfile) != 0) {
		fprintf (stderr, _("%s: warning: can't remove %s: %s"), Prog, mailfile, strerror (errno));
		SYSLOG ((LOG_ERR, "Cannot remove %s: %s", mailfile, strerror (errno)));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id, 0);
#endif
		/* continue */
	}
#ifdef WITH_AUDIT
	else
	{
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id, 1);
	}
#endif
}

/*
 * main - userdel command
 */
int main (int argc, char **argv)
{
	int errors = 0; /* Error in the removal of the home directory */

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

	{
		/*
		 * Parse the command line options.
		 */
		int c;
		static struct option long_options[] = {
			{"force", no_argument, NULL, 'f'},
			{"help", no_argument, NULL, 'h'},
			{"remove", no_argument, NULL, 'r'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c = getopt_long (argc, argv, "fhr",
		                         long_options, NULL)) != -1) {
			switch (c) {
			case 'f':	/* force remove even if not owned by user */
				fflg = true;
				break;
			case 'r':	/* remove home dir and mailbox */
				rflg = true;
				break;
			default:
				usage ();
			}
		}
	}

	if ((optind + 1) != argc) {
		usage ();
	}

	OPENLOG ("userdel");

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("userdel", pampw->pw_name,
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
		exit (E_PW_UPDATE);
	}
#endif				/* USE_PAM */

	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	/*
	 * Start with a quick check to see if the user exists.
	 */
	user_name = argv[argc - 1];
	{
		struct passwd *pwd;
		pwd = getpwnam (user_name); /* local, no need for xgetpwnam */
		if (NULL == pwd) {
			fprintf (stderr, _("%s: user '%s' does not exist\n"),
				 Prog, user_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting user not found",
			              user_name, AUDIT_NO_ID, 0);
#endif
			exit (E_NOTFOUND);
		}
		user_id = pwd->pw_uid;
		user_home = xstrdup (pwd->pw_dir);
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
		if (   !yp_get_default_domain (&nis_domain)
		    && !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr,
				 _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif
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

	if (rflg) {
		remove_mailbox ();
	}
	if (rflg && !fflg && (is_owner (user_id, user_home) == 0)) {
		fprintf (stderr,
			 _("%s: %s not owned by %s, not removing\n"),
			 Prog, user_home, user_name);
		rflg = 0;
		errors++;
		/* continue */
	}

#ifdef EXTRA_CHECK_HOME_DIR
	/* This may be slow, the above should be good enough. */
	if (rflg && !fflg) {
		struct passwd *pwd;
		/*
		 * For safety, refuse to remove the home directory if it
		 * would result in removing some other user's home
		 * directory. Still not perfect so be careful, but should
		 * prevent accidents if someone has /home or / as home
		 * directory...  --marekm
		 */
		setpwent ();
		while ((pwd = getpwent ())) {
			if (strcmp (pwd->pw_name, user_name) == 0) {
				continue;
			}
			if (path_prefix (user_home, pwd->pw_dir)) {
				fprintf (stderr,
					 _
					 ("%s: not removing directory %s (would remove home of user %s)\n"),
					 Prog, user_home, pwd->pw_name);
				rflg = false;
				errors++;
				/* continue */
				break;
			}
		}
		endpwent ();
	}
#endif

	if (rflg) {
		if (remove_tree (user_home) != 0) {
			fprintf (stderr,
				 _("%s: error removing directory %s\n"),
				 Prog, user_home);
			errors++;
			/* continue */
		}
#ifdef WITH_AUDIT
		else
		{
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting home directory",
			              user_name, (unsigned int) user_id, 1);
		}
#endif
	}
#ifdef WITH_AUDIT
	if (0 != errors) {
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting home directory",
		              user_name, AUDIT_NO_ID, 0);
	}
#endif

	/*
	 * Cancel any crontabs or at jobs. Have to do this before we remove
	 * the entry from /etc/passwd.
	 */
	user_cancel (user_name);
	close_files ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

#ifdef USE_PAM
	(void) pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */
	exit ((0 != errors) ? E_HOMEDIR : E_SUCCESS);
	/* NOT REACHED */
}

