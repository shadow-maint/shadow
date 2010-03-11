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
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
#include "defines.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#include "shadowio.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif				/* SHADOWGRP */
#ifdef WITH_TCB
#include <tcb.h>
#include "tcbfuncs.h"
#endif				/* WITH_TCB */
/*@-exitarg@*/
#include "exitcodes.h"

/*
 * exit status values
 */
#define E_PW_UPDATE	1	/* can't update password file */
#define E_NOTFOUND	6	/* specified user doesn't exist */
#define E_USER_BUSY	8	/* user currently logged in */
#define E_GRP_UPDATE	10	/* can't update group file */
#define E_HOMEDIR	12	/* can't remove home directory */

/*
 * Global variables
 */
char *Prog;

static char *user_name;
static uid_t user_id;
static char *user_home;

static bool fflg = false;
static bool rflg = false;

static bool is_shadow_pwd;

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif				/* SHADOWGRP */
static bool pw_locked  = false;
static bool gr_locked   = false;
static bool spw_locked  = false;

/* local function prototypes */
static void usage (int status);
static void update_groups (void);
static void close_files (void);
static void fail_exit (int);
static void open_files (void);
static void update_user (void);
static void user_cancel (const char *);

#ifdef EXTRA_CHECK_HOME_DIR
static bool path_prefix (const char *, const char *);
#endif				/* EXTRA_CHECK_HOME_DIR */
static int is_owner (uid_t, const char *);
static int remove_mailbox (void);
#ifdef WITH_TCB
static int remove_tcbdir (const char *user_name, uid_t user_id);
#endif				/* WITH_TCB */

/*
 * usage - display usage message and exit
 */
static void usage (int status)
{
	fputs (_("Usage: userdel [options] LOGIN\n"
	         "\n"
	         "Options:\n"
	         "  -f, --force                   force removal of files,\n"
	         "                                even if not owned by user\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -r, --remove                  remove home directory and mail spool\n"
	         "\n"), (E_SUCCESS != status) ? stderr : stdout);
	exit (status);
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
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_SUCCESS);
#endif				/* WITH_AUDIT */
		SYSLOG ((LOG_INFO, "delete '%s' from group '%s'\n",
			 user_name, ngrp->gr_name));
	}

	/*
	 * we've removed their name from all the groups above, so
	 * now if they have a group with the same name as their
	 * user name, with no members, we delete it.
	 * FIXME: below, the check for grp->gr_mem[0] is not sufficient.
	 *        We should retrieve the group with gr_locate and check
	 *        that gr_mem is empty.
	 */
	grp = xgetgrnam (user_name);
	if (   (NULL != grp)
	    && getdef_bool ("USERGROUPS_ENAB")
	    && (   (NULL == grp->gr_mem[0])
	        || (   (NULL == grp->gr_mem[1])
	            && (strcmp (grp->gr_mem[0], user_name) == 0)))) {

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
					         _("%s: group %s is the primary group of another user and is not removed.\n"),
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
#endif				/* SHADOWGRP */

#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_GROUP, Prog,
			              "deleting group",
			              grp->gr_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_SUCCESS);
#endif				/* WITH_AUDIT */
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
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_SUCCESS);
#endif				/* WITH_AUDIT */
		SYSLOG ((LOG_INFO, "delete '%s' from shadow group '%s'\n",
			 user_name, nsgrp->sg_name));
	}

	if (   deleted_user_group
	    && (sgr_locate (user_name) != NULL)) {
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
#endif				/* SHADOWGRP */
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
#endif				/* SHADOWGRP */

#ifdef WITH_AUDIT
	audit_logger (AUDIT_DEL_USER, Prog,
	              "deleting user",
	              user_name, (unsigned int) user_id,
	              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */

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
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
		fail_exit (E_PW_UPDATE);
	}
	pw_locked = true;
	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, pw_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "opening password file",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
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
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
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
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
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
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
		fail_exit (E_GRP_UPDATE);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "opening group file",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
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
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked= true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr, _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "opening shadow group file",
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif				/* SHADOWGRP */
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
	if (   is_shadow_pwd
	    && (spw_locate (user_name) != NULL)
	    && (spw_remove (user_name) == 0)) {
		fprintf (stderr,
		         _("%s: cannot remove entry '%s' from %s\n"),
		         Prog, user_name, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_DEL_USER, Prog,
	              "deleting user entries",
	              user_name, (unsigned int) user_id,
	              SHADOW_AUDIT_SUCCESS);
#endif				/* WITH_AUDIT */
	SYSLOG ((LOG_INFO, "delete user '%s'\n", user_name));
}

/* 
 * user_cancel - cancel cron and at jobs
 *
 *	user_cancel calls a script for additional cleanups like removal of
 *	cron, at, or print jobs.
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
		exit (errno == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
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
#endif				/* EXTRA_CHECK_HOME_DIR */

/*
 * is_owner - Check if path is owned by uid
 *
 * Return
 *  1: path exists and is owned by uid
 *  0: path is not owned by uid, or a failure occured
 * -1: path does not exist
 */
static int is_owner (uid_t uid, const char *path)
{
	struct stat st;

	errno = 0;
	if (stat (path, &st) != 0) {
		if ((ENOENT == errno) || (ENOTDIR == errno)) {
			/* The file or directory does not exist */
			return -1;
		} else {
			return 0;
		}
	}
	return (st.st_uid == uid);
}

static int remove_mailbox (void)
{
	const char *maildir;
	char mailfile[1024];
	int i;
	int errors = 0;

	maildir = getdef_str ("MAIL_DIR");
#ifdef MAIL_SPOOL_DIR
	if ((NULL == maildir) && (getdef_str ("MAIL_FILE") == NULL)) {
		maildir = MAIL_SPOOL_DIR;
	}
#endif				/* MAIL_SPOOL_DIR */
	if (NULL == maildir) {
		return 0;
	}
	snprintf (mailfile, sizeof mailfile, "%s/%s", maildir, user_name);
	if (fflg) {
		if (unlink (mailfile) != 0) {
			fprintf (stderr,
			         _("%s: warning: can't remove %s: %s\n"),
			         Prog, mailfile, strerror (errno));
			SYSLOG ((LOG_ERR, "Cannot remove %s: %s", mailfile, strerror (errno)));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting mail file",
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			errors = 1;
			/* continue */
		}
#ifdef WITH_AUDIT
		else
		{
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting mail file",
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_SUCCESS);
		}
#endif				/* WITH_AUDIT */
		return errors;
	}
	i = is_owner (user_id, mailfile);
	if (i == 0) {
		fprintf (stderr,
		         _("%s: %s not owned by %s, not removing\n"),
		         Prog, mailfile, user_name);
		SYSLOG ((LOG_ERR,
		         "%s not owned by %s, not removed",
		         mailfile, strerror (errno)));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
		return 1;
	} else if (i == -1) {
		return 0;		/* mailbox doesn't exist */
	}
	if (unlink (mailfile) != 0) {
		fprintf (stderr,
		         _("%s: warning: can't remove %s: %s\n"),
		         Prog, mailfile, strerror (errno));
		SYSLOG ((LOG_ERR, "Cannot remove %s: %s", mailfile, strerror (errno)));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
		errors = 1;
		/* continue */
	}
#ifdef WITH_AUDIT
	else
	{
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting mail file",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_SUCCESS);
	}
#endif				/* WITH_AUDIT */
	return errors;
}

#ifdef WITH_TCB
static int remove_tcbdir (const char *user_name, uid_t user_id)
{
	char *buf;
	int ret = 0;
	size_t bufsize = (sizeof TCB_DIR) + strlen (user_name) + 2;

	if (!getdef_bool ("USE_TCB"))
		return 0;
	
	buf = malloc (buflen);
	if (NULL == buf) {
		fprintf (stderr, "Can't allocate memory, "
		                 "tcb entry for %s not removed.\n",
		         user_name);
		return 1;
	}
	snprintf (buf, buflen, TCB_DIR "/%s", user_name);
	if (!shadowtcb_drop_priv ()) {
		perror ("shadowtcb_drop_priv");
		free (buf);
		return 1;
	}
	/* Only remove directory contents with dropped privileges.
	 * We will regain them and remove the user's tcb directory afterwards.
	 */
	if (remove_tree (buf, false)) {
		perror ("remove_tree");
		shadowtcb_gain_priv ();
		free (buf);
		return 1;
	}
	shadowtcb_gain_priv ();
	free (buf);
	if (!shadowtcb_remove (user_name)) {
		fprintf (stderr, "Cannot remove tcb files for %s: %s\n",
		         user_name, strerror (errno));
		ret = 1;
	}
	return ret;
}
#endif				/* WITH_TCB */

/*
 * main - userdel command
 */
int main (int argc, char **argv)
{
	int errors = 0; /* Error in the removal of the home directory */

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

#ifdef WITH_AUDIT
	audit_help_open ();
#endif				/* WITH_AUDIT */

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
			case 'h':
				usage (E_SUCCESS);
				break;
			case 'r':	/* remove home dir and mailbox */
				rflg = true;
				break;
			default:
				usage (E_USAGE);
			}
		}
	}

	if ((optind + 1) != argc) {
		usage (E_USAGE);
	}

	OPENLOG ("userdel");

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			exit (E_PW_UPDATE);
		}

		retval = pam_start ("userdel", pampw->pw_name, &conv, &pamh);
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
		exit (E_PW_UPDATE);
	}
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif				/* SHADOWGRP */

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
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			exit (E_NOTFOUND);
		}
		user_id = pwd->pw_uid;
		user_home = xstrdup (pwd->pw_dir);
	}
#ifdef WITH_TCB
	if (!shadowtcb_set_user (user_name))
		exit (E_NOTFOUND);
#endif				/* WITH_TCB */
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
#endif				/* USE_NIS */
	/*
	 * Check to make certain the user isn't logged in.
	 * Note: This is a best effort basis. The user may log in between,
	 * a cron job may be started on her behalf, etc.
	 */
	if (user_busy (user_name, user_id) != 0) {
		fprintf (stderr,
		         _("%s: user %s is currently logged in\n"),
		         Prog, user_name);
		if (!fflg) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_DEL_USER, Prog,
			              "deleting user logged in",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			exit (E_USER_BUSY);
		}
	}

	/*
	 * Do the hard stuff - open the files, create the user entries,
	 * create the home directory, then close and update the files.
	 */
	open_files ();
	update_user ();
	update_groups ();

	if (rflg) {
		errors += remove_mailbox ();
	}
	if (rflg) {
		int home_owned = is_owner (user_id, user_home);
		if (-1 == home_owned) {
			fprintf (stderr,
			         _("%s: %s home directory (%s) not found\n"),
			         Prog, user_name, user_home);
			rflg = 0;
		} else if ((0 == home_owned) && !fflg) {
			fprintf (stderr,
			         _("%s: %s not owned by %s, not removing\n"),
			         Prog, user_home, user_name);
			rflg = 0;
			errors++;
			/* continue */
		}
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
#endif				/* EXTRA_CHECK_HOME_DIR */

	if (rflg) {
		if (remove_tree (user_home, true) != 0) {
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
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_SUCCESS);
		}
#endif				/* WITH_AUDIT */
	}
#ifdef WITH_AUDIT
	if (0 != errors) {
		audit_logger (AUDIT_DEL_USER, Prog,
		              "deleting home directory",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
	}
#endif				/* WITH_AUDIT */

#ifdef WITH_SELINUX
	if (is_selinux_enabled () > 0) {
		const char *args[5];
		args[0] = "/usr/sbin/semanage";
		args[1] = "login";
		args[2] = "-d";
		args[3] = user_name;
		args[4] = NULL;
		safe_system (args[0], args, NULL, 1);
	}
#endif				/* WITH_SELINUX */

	/*
	 * Cancel any crontabs or at jobs. Have to do this before we remove
	 * the entry from /etc/passwd.
	 */
	user_cancel (user_name);
	close_files ();

#ifdef WITH_TCB
	errors += remove_tcbdir (user_name, user_id);
#endif				/* WITH_TCB */

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	return ((0 != errors) ? E_HOMEDIR : E_SUCCESS);
}

