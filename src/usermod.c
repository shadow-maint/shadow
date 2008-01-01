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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <lastlog.h>
#include <pwd.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "chkname.h"
#include "defines.h"
#include "faillog.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
#include "shadowio.h"
/*
 * exit status values
 * for E_GRP_UPDATE and E_NOSPACE (not used yet), other update requests
 * will be implemented (as documented in the Solaris 2.x man page).
 */
#define E_SUCCESS	0	/* success */
#define E_PW_UPDATE	1	/* can't update password file */
#define E_USAGE		2	/* invalid command syntax */
#define E_BAD_ARG	3	/* invalid argument to option */
#define E_UID_IN_USE	4	/* UID already in use (and no -o) */
/* #define E_BAD_PWFILE	5	   passwd file contains errors */
#define E_NOTFOUND	6	/* specified user/group doesn't exist */
#define E_USER_BUSY	8	/* user to modify is logged in */
#define E_NAME_IN_USE	9	/* username already in use */
#define E_GRP_UPDATE	10	/* can't update group file */
/* #define E_NOSPACE	11	   insufficient space to move home dir */
#define E_HOMEDIR	12	/* unable to complete home dir move */
#define	VALID(s)	(strcspn (s, ":\n") == strlen (s))
/*
 * Global variables
 */
static char *user_name;
static char *user_newname;
static char *user_pass;
static uid_t user_id;
static uid_t user_newid;
static gid_t user_gid;
static gid_t user_newgid;
static char *user_comment;
static char *user_home;
static char *user_newhome;
static char *user_shell;
static long user_expire;
static long user_inactive;
static long sys_ngroups;
static char **user_groups;	/* NULL-terminated list */

#ifdef WITH_AUDIT
static char *user_newcomment;	/* Audit */
static char *user_newshell;	/* Audit */
static long user_newexpire;	/* Audit */
static long user_newinactive;	/* Audit */
#endif

static char *Prog;

static int
    aflg = 0,			/* append to existing secondary group set */
    cflg = 0,			/* new comment (GECOS) field */
    dflg = 0,			/* new home directory */
    eflg = 0,			/* days since 1970-01-01 when account becomes expired */
    fflg = 0,			/* days until account with expired password is locked */
    gflg = 0,			/* new primary group ID */
    Gflg = 0,			/* new secondary group set */
    Lflg = 0,			/* lock the password */
    lflg = 0,			/* new user name */
    mflg = 0,			/* create user's home directory if it doesn't exist */
    oflg = 0,			/* permit non-unique user ID to be specified with -u */
    pflg = 0,			/* new encrypted password */
    sflg = 0,			/* new shell program */
    uflg = 0,			/* specify new user ID */
    Uflg = 0;			/* unlock the password */

static int is_shadow_pwd;

#ifdef SHADOWGRP
static int is_shadow_grp;
#endif

static int pw_locked  = 0;
static int spw_locked = 0;
static int gr_locked  = 0;
#ifdef SHADOWGRP
static int sgr_locked = 0;
#endif


/* local function prototypes */
static int get_groups (char *);
static void usage (void);
static void new_pwent (struct passwd *);

static void new_spent (struct spwd *);
static void fail_exit (int);
static void update_group (void);

#ifdef SHADOWGRP
static void update_gshadow (void);
#endif
static void grp_update (void);

static long get_number (const char *);
static uid_t get_id (const char *);
static void process_flags (int, char **);
static void close_files (void);
static void open_files (void);
static void usr_update (void);
static void move_home (void);
static void update_files (void);

#ifndef NO_MOVE_MAILBOX
static void move_mailbox (void);
#endif

/*
 * Had to move this over from useradd.c since we have groups named
 * "56k-family"... ergh.
 * --Pac.
 */
static struct group *getgr_nam_gid (const char *grname)
{
	long val;
	char *errptr;

	val = strtol (grname, &errptr, 10);
	if (*grname != '\0' && *errptr == '\0' && errno != ERANGE && val >= 0)
		return xgetgrgid (val);
	return xgetgrnam (grname);
}

/*
 * get_groups - convert a list of group names to an array of group IDs
 *
 *	get_groups() takes a comma-separated list of group names and
 *	converts it to a NULL-terminated array. Any unknown group names are
 *	reported as errors.
 */
static int get_groups (char *list)
{
	char *cp;
	const struct group *grp;
	int errors = 0;
	int ngroups = 0;

	/*
	 * Initialize the list to be empty
	 */
	user_groups[0] = (char *) 0;

	if (!*list)
		return 0;

	/*
	 * So long as there is some data to be converted, strip off each
	 * name and look it up. A mix of numerical and string values for
	 * group identifiers is permitted.
	 */
	do {
		/*
		 * Strip off a single name from the list
		 */
		if ((cp = strchr (list, ',')))
			*cp++ = '\0';

		/*
		 * Names starting with digits are treated as numerical GID
		 * values, otherwise the string is looked up as is.
		 */
		grp = getgr_nam_gid (list);

		/*
		 * There must be a match, either by GID value or by
		 * string name.
		 */
		if (!grp) {
			fprintf (stderr, _("%s: unknown group %s\n"),
				 Prog, list);
			errors++;
		}
		list = cp;

		/*
		 * If the group doesn't exist, don't dump core. Instead,
		 * try the next one.  --marekm
		 */
		if (!grp)
			continue;

#ifdef	USE_NIS
		/*
		 * Don't add this group if they are an NIS group. Tell the
		 * user to go to the server for this group.
		 */
		if (__isgrNIS ()) {
			fprintf (stderr,
				 _("%s: group '%s' is a NIS group.\n"),
				 Prog, grp->gr_name);
			continue;
		}
#endif

		if (ngroups == sys_ngroups) {
			fprintf (stderr,
				 _
				 ("%s: too many groups specified (max %d).\n"),
				 Prog, ngroups);
			break;
		}

		/*
		 * Add the group name to the user's list of groups.
		 */
		user_groups[ngroups++] = xstrdup (grp->gr_name);
	} while (list);

	user_groups[ngroups] = (char *) 0;

	/*
	 * Any errors in finding group names are fatal
	 */
	if (errors)
		return -1;

	return 0;
}

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: usermod [options] LOGIN\n"
			   "\n"
			   "Options:\n"
			   "  -c, --comment COMMENT         new value of the GECOS field\n"
			   "  -d, --home HOME_DIR           new home directory for the user account\n"
			   "  -e, --expiredate EXPIRE_DATE  set account expiration date to EXPIRE_DATE\n"
			   "  -f, --inactive INACTIVE       set password inactive after expiration\n"
			   "                                to INACTIVE\n"
			   "  -g, --gid GROUP               force use GROUP as new primary group\n"
			   "  -G, --groups GROUPS           new list of supplementary GROUPS\n"
			   "  -a, --append                  append the user to the supplemental GROUPS\n"
			   "                                mentioned by the -G option without removing\n"
			   "                                him/her from other groups\n"
			   "  -h, --help                    display this help message and exit\n"
			   "  -l, --login NEW_LOGIN         new value of the login name\n"
			   "  -L, --lock                    lock the user account\n"
			   "  -m, --move-home               move contents of the home directory to the\n"
			   "                                new location (use only with -d)\n"
			   "  -o, --non-unique              allow using duplicate (non-unique) UID\n"
			   "  -p, --password PASSWORD       use encrypted password for the new password\n"
			   "  -s, --shell SHELL             new login shell for the user account\n"
			   "  -u, --uid UID                 new UID for the user account\n"
			   "  -U, --unlock                  unlock the user account\n"
			   "\n"));
	exit (E_USAGE);
}

/*
 * update encrypted password string (for both shadow and non-shadow
 * passwords)
 */
static char *new_pw_passwd (char *pw_pass, const char *pw_name)
{
	if (Lflg && pw_pass[0] != '!') {
		char *buf = xmalloc (strlen (pw_pass) + 2);

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "updating passwd",
			      user_newname, user_newid, 0);
#endif
		SYSLOG ((LOG_INFO, "lock user `%s' password", pw_name));
		strcpy (buf, "!");
		strcat (buf, pw_pass);
		pw_pass = buf;
	} else if (Uflg && pw_pass[0] == '!') {
		char *s;

		if (pw_pass[1] == '\0') {
			fprintf (stderr,
				 _("%s: unlocking the user would result in a passwordless account.\n"
				   "You should set a password with usermod -p to unlock this user account.\n"),
				 Prog);
			return pw_pass;
		}

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "updating password",
			      user_newname, user_newid, 0);
#endif
		SYSLOG ((LOG_INFO, "unlock user `%s' password", pw_name));
		s = pw_pass;
		while (*s) {
			*s = *(s + 1);
			s++;
		}
	} else if (pflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing password",
			      user_newname, user_newid, 1);
#endif
		SYSLOG ((LOG_INFO, "change user `%s' password", pw_name));
		pw_pass = xstrdup (user_pass);
	}
	return pw_pass;
}

/*
 * new_pwent - initialize the values in a password file entry
 *
 *	new_pwent() takes all of the values that have been entered and fills
 *	in a (struct passwd) with them.
 */
static void new_pwent (struct passwd *pwent)
{
	if (lflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing name",
			      user_newname, user_newid, 1);
#endif
		SYSLOG ((LOG_INFO, "change user name `%s' to `%s'",
			 pwent->pw_name, user_newname));
		pwent->pw_name = xstrdup (user_newname);
	}
	if (!is_shadow_pwd)
		pwent->pw_passwd =
		    new_pw_passwd (pwent->pw_passwd, pwent->pw_name);

	if (uflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing uid",
			      user_newname, user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
			 "change user `%s' UID from `%d' to `%d'",
			 pwent->pw_name, pwent->pw_uid, user_newid));
		pwent->pw_uid = user_newid;
	}
	if (gflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "changing primary group", user_newname,
			      user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
			 "change user `%s' GID from `%d' to `%d'",
			 pwent->pw_name, pwent->pw_gid, user_newgid));
		pwent->pw_gid = user_newgid;
	}
	if (cflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing comment",
			      user_newname, user_newid, 1);
		pwent->pw_gecos = user_newcomment;
#else
		pwent->pw_gecos = user_comment;
#endif
	}

	if (dflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "changing home directory", user_newname,
			      user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
			 "change user `%s' home from `%s' to `%s'",
			 pwent->pw_name, pwent->pw_dir, user_newhome));
		pwent->pw_dir = user_newhome;
	}
	if (sflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing user shell",
			      user_newname, user_newid, 1);
		SYSLOG ((LOG_INFO, "change user `%s' shell from `%s' to `%s'",
			 pwent->pw_name, pwent->pw_shell, user_newshell));
		pwent->pw_shell = user_newshell;
#else
		SYSLOG ((LOG_INFO,
			 "change user `%s' shell from `%s' to `%s'",
			 pwent->pw_name, pwent->pw_shell, user_shell));
		pwent->pw_shell = user_shell;
#endif
	}
}

/*
 * new_spent - initialize the values in a shadow password file entry
 *
 *	new_spent() takes all of the values that have been entered and fills
 *	in a (struct spwd) with them.
 */
static void new_spent (struct spwd *spent)
{
	if (lflg)
		spent->sp_namp = xstrdup (user_newname);

	if (fflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "changing inactive days", user_newname,
			      user_newid, 1);
		SYSLOG ((LOG_INFO,
			 "change user `%s' inactive from `%ld' to `%ld'",
			 spent->sp_namp, spent->sp_inact, user_newinactive));
		spent->sp_inact = user_newinactive;
#else

		SYSLOG ((LOG_INFO,
			 "change user `%s' inactive from `%ld' to `%ld'",
			 spent->sp_namp, spent->sp_inact, user_inactive));
		spent->sp_inact = user_inactive;
#endif
	}
	if (eflg) {
		/* XXX - dates might be better than numbers of days.  --marekm */
#ifdef WITH_AUDIT
		if (audit_fd >= 0) {
			time_t exp_t;
			struct tm *exp_tm;
			char new_exp[16], old_exp[16];

			if (user_newexpire == -1)
				new_exp[0] = '\0';
			else {
				exp_t = user_newexpire * DAY;
				exp_tm = gmtime (&exp_t);
#ifdef HAVE_STRFTIME
				strftime (new_exp, sizeof (new_exp), "%Y-%m-%d",
					  exp_tm);
#else
				memset (new_exp, 0, sizeof (new_exp));
				snprintf (new_exp, sizeof (new_exp) - 1,
					  "%04i-%02i-%02i",
					  exp_tm->tm_year + 1900,
					  exp_tm->tm_mon + 1, exp_tm->tm_mday);
#endif
			}

			if (user_expire == -1)
				old_exp[0] = '\0';
			else {
				exp_t = user_expire * DAY;
				exp_tm = gmtime (&exp_t);
#ifdef HAVE_STRFTIME
				strftime (old_exp, sizeof (old_exp), "%Y-%m-%d",
					  exp_tm);
#else
				memset (old_exp, 0, sizeof (old_exp));
				snprintf (old_exp, sizeof (old_exp) - 1,
					  "%04i-%02i-%02i",
					  exp_tm->tm_year + 1900,
					  exp_tm->tm_mon + 1, exp_tm->tm_mday);
#endif
			}
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "changing expiration date", user_newname,
				      user_newid, 1);
		}

		SYSLOG ((LOG_INFO,
			 "change user `%s' expiration from `%ld' to `%ld'",
			 spent->sp_namp, spent->sp_expire, user_newexpire));
		spent->sp_expire = user_newexpire;
#else
		SYSLOG ((LOG_INFO,
			 "change user `%s' expiration from `%ld' to `%ld'",
			 spent->sp_namp, spent->sp_expire, user_expire));
		spent->sp_expire = user_expire;
#endif
	}
	spent->sp_pwdp = new_pw_passwd (spent->sp_pwdp, spent->sp_namp);
	if (pflg)
		spent->sp_lstchg = time ((time_t *) 0) / SCALE;
}

/*
 * fail_exit - exit with an error code after unlocking files
 */
static void fail_exit (int code)
{
	if (gr_locked)
		gr_unlock ();
#ifdef	SHADOWGRP
	if (sgr_locked)
		sgr_unlock ();
#endif
	if (spw_locked)
		spw_unlock ();
	if (pw_locked)
		pw_unlock ();

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "modifying account",
		      user_name, -1, 0);
#endif
	exit (code);
}


static void update_group (void)
{
	int is_member;
	int was_member;
	int changed;
	const struct group *grp;
	struct group *ngrp;

	changed = 0;

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */
	while ((grp = gr_next ())) {
		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		was_member = is_on_list (grp->gr_mem, user_name);
		is_member = Gflg && is_on_list (user_groups, grp->gr_name);

		if (!was_member && !is_member)
			continue;

		ngrp = __gr_dup (grp);
		if (!ngrp) {
			fprintf (stderr,
				 _("%s: Out of memory. Cannot update the group database.\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}

		if (was_member && (!Gflg || is_member)) {
			if (lflg) {
				ngrp->gr_mem = del_list (ngrp->gr_mem,
							 user_name);
				ngrp->gr_mem = add_list (ngrp->gr_mem,
							 user_newname);
				changed = 1;
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					      "changing group member",
					      user_newname, -1, 1);
#endif
				SYSLOG ((LOG_INFO,
					 "change `%s' to `%s' in group `%s'",
					 user_name, user_newname,
					 ngrp->gr_name));
			}
		} else if (was_member && !aflg && Gflg && !is_member) {
			ngrp->gr_mem = del_list (ngrp->gr_mem, user_name);
			changed = 1;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "removing group member", user_name, -1,
				      1);
#endif
			SYSLOG ((LOG_INFO, "delete `%s' from group `%s'",
				 user_name, ngrp->gr_name));
		} else if (!was_member && Gflg && is_member) {
			ngrp->gr_mem = add_list (ngrp->gr_mem,
						 lflg ? user_newname :
						 user_name);
			changed = 1;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "adding user to group", user_name, -1, 1);
#endif
			SYSLOG ((LOG_INFO, "add `%s' to group `%s'",
				 lflg ? user_newname : user_name,
				 ngrp->gr_name));
		}
		if (!changed)
			continue;

		changed = 0;
		if (!gr_update (ngrp)) {
			fprintf (stderr,
				 _("%s: error adding new group entry\n"), Prog);
			SYSLOG ((LOG_ERR, "error adding new group entry"));
			fail_exit (E_GRP_UPDATE);
		}
	}
}

#ifdef SHADOWGRP
static void update_gshadow (void)
{
	int is_member;
	int was_member;
	int was_admin;
	int changed;
	const struct sgrp *sgrp;
	struct sgrp *nsgrp;

	changed = 0;

	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of.
	 */
	while ((sgrp = sgr_next ())) {

		/*
		 * See if the user was a member of this group
		 */
		was_member = is_on_list (sgrp->sg_mem, user_name);

		/*
		 * See if the user was an administrator of this group
		 */
		was_admin = is_on_list (sgrp->sg_adm, user_name);

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		is_member = Gflg && is_on_list (user_groups, sgrp->sg_name);

		if (!was_member && !was_admin && !is_member)
			continue;

		nsgrp = __sgr_dup (sgrp);
		if (!nsgrp) {
			fprintf (stderr,
				 _("%s: Out of memory. Cannot update the shadow group database.\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}

		if (was_admin && lflg) {
			nsgrp->sg_adm = del_list (nsgrp->sg_adm, user_name);
			nsgrp->sg_adm = add_list (nsgrp->sg_adm, user_newname);
			changed = 1;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "changing admin name in shadow group",
				      user_name, -1, 1);
#endif
			SYSLOG ((LOG_INFO,
				 "change admin `%s' to `%s' in shadow group `%s'",
				 user_name, user_newname, nsgrp->sg_name));
		}
		if (was_member && (!Gflg || is_member)) {
			if (lflg) {
				nsgrp->sg_mem = del_list (nsgrp->sg_mem,
							  user_name);
				nsgrp->sg_mem = add_list (nsgrp->sg_mem,
							  user_newname);
				changed = 1;
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					      "changing member in shadow group",
					      user_name, -1, 1);
#endif
				SYSLOG ((LOG_INFO,
					 "change `%s' to `%s' in shadow group `%s'",
					 user_name, user_newname,
					 nsgrp->sg_name));
			}
		} else if (was_member && !aflg && Gflg && !is_member) {
			nsgrp->sg_mem = del_list (nsgrp->sg_mem, user_name);
			changed = 1;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "removing user from shadow group",
				      user_name, -1, 1);
#endif
			SYSLOG ((LOG_INFO,
				 "delete `%s' from shadow group `%s'",
				 user_name, nsgrp->sg_name));
		} else if (!was_member && Gflg && is_member) {
			nsgrp->sg_mem = add_list (nsgrp->sg_mem,
						  lflg ? user_newname :
						  user_name);
			changed = 1;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "adding user to shadow group",
				      user_newname, -1, 1);
#endif
			SYSLOG ((LOG_INFO, "add `%s' to shadow group `%s'",
				 lflg ? user_newname : user_name,
				 nsgrp->sg_name));
		}
		if (!changed)
			continue;

		changed = 0;

		/* 
		 * Update the group entry to reflect the changes.
		 */
		if (!sgr_update (nsgrp)) {
			fprintf (stderr,
				 _("%s: error adding new shadow group entry\n"), Prog);
			SYSLOG ((LOG_ERR, "error adding shadow group entry"));
			fail_exit (E_GRP_UPDATE);
		}
	}
}
#endif				/* SHADOWGRP */

/*
 * grp_update - add user to secondary group set
 *
 *	grp_update() takes the secondary group set given in user_groups and
 *	adds the user to each group given by that set.
 */
static void grp_update (void)
{
	update_group ();
#ifdef SHADOWGRP
	if (is_shadow_grp)
		update_gshadow ();
#endif
}

static long get_number (const char *numstr)
{
	long val;
	char *errptr;

	val = strtol (numstr, &errptr, 10);
	if (*errptr || errno == ERANGE) {
		fprintf (stderr, _("%s: invalid numeric argument '%s'\n"), Prog,
			 numstr);
		exit (E_BAD_ARG);
	}
	return val;
}

static uid_t get_id (const char *uidstr)
{
	long val;
	char *errptr;

	val = strtol (uidstr, &errptr, 10);
	if (*errptr || errno == ERANGE || val < 0) {
		fprintf (stderr, _("%s: invalid numeric argument '%s'\n"), Prog,
			 uidstr);
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
	const struct group *grp;

	int anyflag = 0;

	if (argc == 1 || argv[argc - 1][0] == '-')
		usage ();

	{
		const struct passwd *pwd;
		/* local, no need for xgetpwnam */
		if (!(pwd = getpwnam (argv[argc - 1]))) {
			fprintf (stderr, _("%s: user %s does not exist\n"),
				 Prog, argv[argc - 1]);
			exit (E_NOTFOUND);
		}

		user_name = argv[argc - 1];
		user_id = pwd->pw_uid;
		user_gid = pwd->pw_gid;
		user_comment = xstrdup (pwd->pw_gecos);
		user_home = xstrdup (pwd->pw_dir);
		user_shell = xstrdup (pwd->pw_shell);
	}
#ifdef WITH_AUDIT
	user_newname = user_name;
	user_newid = user_id;
	user_newgid = user_gid;
	user_newcomment = user_comment;
	user_newhome = user_home;
	user_newshell = user_shell;
#endif

#ifdef	USE_NIS
	/*
	 * Now make sure it isn't an NIS user.
	 */
	if (__ispwNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr, _("%s: user %s is a NIS user\n"),
			 Prog, user_name);

		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr, _("%s: %s is the NIS master\n"),
				 Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif

	{
		const struct spwd *spwd = NULL;
		/* local, no need for xgetspnam */
		if (is_shadow_pwd && (spwd = getspnam (user_name))) {
			user_expire = spwd->sp_expire;
			user_inactive = spwd->sp_inact;
#ifdef WITH_AUDIT
			user_newexpire = user_expire;
			user_newinactive = user_inactive;
#endif
		}
	}

	{
		/*
		 * Parse the command line options.
		 */
		int c;
		static struct option long_options[] = {
			{"append", no_argument, NULL, 'a'},
			{"comment", required_argument, NULL, 'c'},
			{"home", required_argument, NULL, 'd'},
			{"expiredate", required_argument, NULL, 'e'},
			{"inactive", required_argument, NULL, 'f'},
			{"gid", required_argument, NULL, 'g'},
			{"groups", required_argument, NULL, 'G'},
			{"help", no_argument, NULL, 'h'},
			{"login", required_argument, NULL, 'l'},
			{"lock", no_argument, NULL, 'L'},
			{"move-home", no_argument, NULL, 'm'},
			{"non-unique", no_argument, NULL, 'o'},
			{"password", required_argument, NULL, 'p'},
			{"shell", required_argument, NULL, 's'},
			{"uid", required_argument, NULL, 'u'},
			{"unlock", no_argument, NULL, 'U'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c =
			getopt_long (argc, argv, "ac:d:e:f:g:G:hl:Lmop:s:u:U",
				     long_options, NULL)) != -1) {
			switch (c) {
			case 'a':
				aflg++;
				break;
			case 'c':
				if (!VALID (optarg)) {
					fprintf (stderr,
						 _("%s: invalid field '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
#ifdef WITH_AUDIT
				user_newcomment = optarg;
#else
				user_comment = optarg;
#endif
				cflg++;
				break;
			case 'd':
				if (!VALID (optarg)) {
					fprintf (stderr,
						 _("%s: invalid field '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				dflg++;
				user_newhome = optarg;
				break;
			case 'e':
				if (*optarg) {
#ifdef WITH_AUDIT
					user_newexpire = strtoday (optarg);
					if (user_newexpire == -1) {
#else /* } */
					user_expire = strtoday (optarg);
					if (user_expire == -1) {
#endif
						fprintf (stderr,
							 _
							 ("%s: invalid date '%s'\n"),
							 Prog, optarg);
						exit (E_BAD_ARG);
					}
#ifdef WITH_AUDIT
					user_newexpire *= DAY / SCALE;
#else
					user_expire *= DAY / SCALE;
#endif
				} else
#ifdef WITH_AUDIT
					user_newexpire = -1;
#else
					user_expire = -1;
#endif
				eflg++;
				break;
			case 'f':
#ifdef WITH_AUDIT
				user_newinactive = get_number (optarg);
#else
				user_inactive = get_number (optarg);
#endif
				fflg++;
				break;
			case 'g':
				grp = getgr_nam_gid (optarg);
				if (!grp) {
					fprintf (stderr,
						 _("%s: unknown group %s\n"),
						 Prog, optarg);
					exit (E_NOTFOUND);
				}
				user_newgid = grp->gr_gid;
				gflg++;
				break;
			case 'G':
				if (get_groups (optarg))
					exit (E_NOTFOUND);
				Gflg++;
				break;
			case 'l':
				if (!check_user_name (optarg)) {
					fprintf (stderr,
						 _("%s: invalid field '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}

				/*
				 * If the name does not really change, we mustn't
				 * set the flag as this will cause rather serious
				 * problems later!
				 */
				if (strcmp (user_name, optarg))
					lflg++;

				user_newname = optarg;
				break;
			case 'L':
				Lflg++;
				break;
			case 'm':
				mflg++;
				break;
			case 'o':
				oflg++;
				break;
			case 'p':
				user_pass = optarg;
				pflg++;
				break;
			case 's':
				if (!VALID (optarg)) {
					fprintf (stderr,
						 _("%s: invalid field '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
#ifdef WITH_AUDIT
				user_newshell = optarg;
#else
				user_shell = optarg;
#endif
				sflg++;
				break;
			case 'u':
				user_newid = get_id (optarg);
				uflg++;
				break;
			case 'U':
				Uflg++;
				break;
			default:
				usage ();
			}
			anyflag++;
		}
	}

	if (anyflag == 0) {
		fprintf (stderr, _("%s: no flags given\n"), Prog);
		exit (E_USAGE);
	}
	if (!is_shadow_pwd && (eflg || fflg)) {
		fprintf (stderr,
			 _
			 ("%s: shadow passwords required for -e and -f\n"),
			 Prog);
		exit (E_USAGE);
	}

	if (optind != argc - 1)
		usage ();

	if (aflg && (!Gflg)) {
		fprintf (stderr,
			 _("%s: %s flag is ONLY allowed with the %s flag\n"),
			 Prog, "-a", "-G");
		usage ();
		exit (E_USAGE);
	}

	if ((Lflg && (pflg || Uflg)) || (pflg && Uflg)) {
		fprintf (stderr,
			 _("%s: the -L, -p, and -U flags are exclusive\n"),
			 Prog);
		usage ();
		exit (E_USAGE);
	}

	if (oflg && !uflg) {
		fprintf (stderr,
			 _("%s: %s flag is ONLY allowed with the %s flag\n"),
			 Prog, "-o", "-u");
		usage ();
		exit (E_USAGE);
	}

	if (mflg && !dflg) {
		fprintf (stderr,
			 _("%s: %s flag is ONLY allowed with the %s flag\n"),
			 Prog, "-m", "-d");
		usage ();
		exit (E_USAGE);
	}

	if (dflg && strcmp (user_home, user_newhome) == 0)
		dflg = mflg = 0;

	if (uflg && user_id == user_newid)
		uflg = oflg = 0;

	/* local, no need for xgetpwnam */
	if (lflg && getpwnam (user_newname)) {
		fprintf (stderr, _("%s: user %s exists\n"), Prog, user_newname);
		exit (E_NAME_IN_USE);
	}

	/* local, no need for xgetpwuid */
	if (uflg && !oflg && getpwuid (user_newid)) {
		fprintf (stderr, _("%s: uid %lu is not unique\n"),
			 Prog, (unsigned long) user_newid);
		exit (E_UID_IN_USE);
	}
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this new
 *	user. This causes any modified entries to be written out.
 */
static void close_files (void)
{
	if (!pw_close ()) {
		fprintf (stderr, _("%s: cannot rewrite password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && !spw_close ()) {
		fprintf (stderr,
			 _("%s: cannot rewrite shadow password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}

	if (Gflg || lflg) {
		if (!gr_close ()) {
			fprintf (stderr, _("%s: cannot rewrite group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
#ifdef SHADOWGRP
		if (is_shadow_grp && !sgr_close ()) {
			fprintf (stderr,
				 _("%s: cannot rewrite shadow group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
		if (is_shadow_grp)
			sgr_unlock ();
#endif
		gr_unlock ();
	}

	if (is_shadow_pwd)
		spw_unlock ();
	pw_unlock ();

	pw_locked = 0;
	spw_locked = 0;
	gr_locked = 0;
	sgr_locked = 0;

	/*
	 * Close the DBM and/or flat files
	 */
	endpwent ();
	endspent ();
	endgrent ();
#ifdef	SHADOWGRP
	endsgent ();
#endif
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
		fail_exit (E_PW_UPDATE);
	}
	pw_locked = 1;
	if (!pw_open (O_RDWR)) {
		fprintf (stderr, _("%s: unable to open password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && !spw_lock ()) {
		fprintf (stderr,
			 _("%s: cannot lock shadow password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}
	spw_locked = 1;
	if (is_shadow_pwd && !spw_open (O_RDWR)) {
		fprintf (stderr,
			 _("%s: cannot open shadow password file\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}

	if (Gflg || lflg) {
		/*
		 * Lock and open the group file. This will load all of the
		 * group entries.
		 */
		if (!gr_lock ()) {
			fprintf (stderr, _("%s: error locking group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
		gr_locked = 1;
		if (!gr_open (O_RDWR)) {
			fprintf (stderr, _("%s: error opening group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
#ifdef SHADOWGRP
		if (is_shadow_grp && !sgr_lock ()) {
			fprintf (stderr,
				 _("%s: error locking shadow group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked = 1;
		if (is_shadow_grp && !sgr_open (O_RDWR)) {
			fprintf (stderr,
				 _("%s: error opening shadow group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
#endif
	}
}

/*
 * usr_update - create the user entries
 *
 *	usr_update() creates the password file entries for this user and
 *	will update the group entries if required.
 */
static void usr_update (void)
{
	struct passwd pwent;
	const struct passwd *pwd;

	struct spwd spent;
	const struct spwd *spwd = NULL;

	/*
	 * Locate the entry in /etc/passwd, which MUST exist.
	 */
	pwd = pw_locate (user_name);
	if (!pwd) {
		fprintf (stderr, _("%s: %s not found in /etc/passwd\n"),
			 Prog, user_name);
		fail_exit (E_NOTFOUND);
	}
	pwent = *pwd;
	new_pwent (&pwent);


	/* 
	 * Locate the entry in /etc/shadow. It doesn't have to exist, and
	 * won't be created if it doesn't.
	 */
	if (is_shadow_pwd && (spwd = spw_locate (user_name))) {
		spent = *spwd;
		new_spent (&spent);
	}

	if (lflg || uflg || gflg || cflg || dflg || sflg || pflg
	    || Lflg || Uflg) {
		if (!pw_update (&pwent)) {
			fprintf (stderr,
				 _("%s: error changing password entry\n"),
				 Prog);
			fail_exit (E_PW_UPDATE);
		}
		if (lflg && !pw_remove (user_name)) {
			fprintf (stderr,
				 _("%s: error removing password entry\n"),
				 Prog);
			fail_exit (E_PW_UPDATE);
		}
	}
	if (spwd && (lflg || eflg || fflg || pflg || Lflg || Uflg)) {
		if (!spw_update (&spent)) {
			fprintf (stderr,
				 _
				 ("%s: error adding new shadow password entry\n"),
				 Prog);
			fail_exit (E_PW_UPDATE);
		}
		if (lflg && !spw_remove (user_name)) {
			fprintf (stderr,
				 _
				 ("%s: error removing shadow password entry\n"),
				 Prog);
			fail_exit (E_PW_UPDATE);
		}
	}
}

/*
 * move_home - move the user's home directory
 *
 *	move_home() moves the user's home directory to a new location. The
 *	files will be copied if the directory cannot simply be renamed.
 */
static void move_home (void)
{
	struct stat sb;

	if (mflg && stat (user_home, &sb) == 0) {
		/*
		 * Don't try to move it if it is not a directory
		 * (but /dev/null for example).  --marekm
		 */
		if (!S_ISDIR (sb.st_mode))
			return;

		if (access (user_newhome, F_OK) == 0) {
			fprintf (stderr, _("%s: directory %s exists\n"),
				 Prog, user_newhome);
			fail_exit (E_HOMEDIR);
		} else if (rename (user_home, user_newhome)) {
			if (errno == EXDEV) {
				if (mkdir (user_newhome, sb.st_mode & 0777)) {
					fprintf (stderr,
						 _
						 ("%s: can't create %s\n"),
						 Prog, user_newhome);
				}
				if (chown (user_newhome, sb.st_uid, sb.st_gid)) {
					fprintf (stderr,
						 _("%s: can't chown %s\n"),
						 Prog, user_newhome);
					rmdir (user_newhome);
					fail_exit (E_HOMEDIR);
				}
				if (copy_tree (user_home, user_newhome,
					       uflg ? (long int)user_newid : -1,
					       gflg ? (long int)user_newgid : -1) == 0) {
					if (remove_tree (user_home) != 0 ||
					    rmdir (user_home) != 0)
						fprintf (stderr,
							 _
							 ("%s: warning: failed to completely remove old home directory %s"),
							 Prog, user_home);
#ifdef WITH_AUDIT
					audit_logger (AUDIT_USER_CHAUTHTOK,
						      Prog,
						      "moving home directory",
						      user_newname, user_newid,
						      1);
#endif
					return;
				}

				(void) remove_tree (user_newhome);
				(void) rmdir (user_newhome);
			}
			fprintf (stderr,
				 _
				 ("%s: cannot rename directory %s to %s\n"),
				 Prog, user_home, user_newhome);
			fail_exit (E_HOMEDIR);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "moving home directory", user_newname, user_newid,
			      1);
#endif
	}
	if (uflg || gflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "changing home directory owner", user_newname,
			      user_newid, 1);
#endif
		chown (dflg ? user_newhome : user_home,
		       uflg ? user_newid : user_id,
		       gflg ? user_newgid : user_gid);
	}
}

/*
 * update_files - update the lastlog and faillog files
 */
static void update_files (void)
{
	struct lastlog ll;
	struct faillog fl;
	int fd;

	/*
	 * Relocate the "lastlog" entries for the user. The old entry is
	 * left alone in case the UID was shared. It doesn't hurt anything
	 * to just leave it be.
	 */
	if ((fd = open (LASTLOG_FILE, O_RDWR)) != -1) {
		lseek (fd, (off_t) user_id * sizeof ll, SEEK_SET);
		if (read (fd, (char *) &ll, sizeof ll) == sizeof ll) {
			lseek (fd, (off_t) user_newid * sizeof ll, SEEK_SET);
			write (fd, (char *) &ll, sizeof ll);
		}
		close (fd);
	}

	/*
	 * Relocate the "faillog" entries in the same manner.
	 */
	if ((fd = open (FAILLOG_FILE, O_RDWR)) != -1) {
		lseek (fd, (off_t) user_id * sizeof fl, SEEK_SET);
		if (read (fd, (char *) &fl, sizeof fl) == sizeof fl) {
			lseek (fd, (off_t) user_newid * sizeof fl, SEEK_SET);
			write (fd, (char *) &fl, sizeof fl);
		}
		close (fd);
	}
}

#ifndef NO_MOVE_MAILBOX
/*
 * This is the new and improved code to carefully chown/rename the user's
 * mailbox. Maybe I am too paranoid but the mail spool dir sometimes
 * happens to be mode 1777 (this makes mail user agents work without
 * being setgid mail, but is NOT recommended; they all should be fixed
 * to use movemail).  --marekm
 */
static void move_mailbox (void)
{
	const char *maildir;
	char mailfile[1024], newmailfile[1024];
	int fd;
	struct stat st;

	maildir = getdef_str ("MAIL_DIR");
#ifdef MAIL_SPOOL_DIR
	if (!maildir && !getdef_str ("MAIL_FILE"))
		maildir = MAIL_SPOOL_DIR;
#endif
	if (!maildir)
		return;

	/*
	 * O_NONBLOCK is to make sure open won't hang on mandatory locks.
	 * We do fstat/fchown to make sure there are no races (someone
	 * replacing /var/spool/mail/luser with a hard link to /etc/passwd
	 * between stat and chown).  --marekm
	 */
	snprintf (mailfile, sizeof mailfile, "%s/%s", maildir, user_name);
	fd = open (mailfile, O_RDONLY | O_NONBLOCK, 0);
	if (fd < 0) {
		/* no need for warnings if the mailbox doesn't exist */
		if (errno != ENOENT)
			perror (mailfile);
		return;
	}
	if (fstat (fd, &st) < 0) {
		perror ("fstat");
		close (fd);
		return;
	}
	if (st.st_uid != user_id) {
		/* better leave it alone */
		fprintf (stderr, _("%s: warning: %s not owned by %s\n"),
			 Prog, mailfile, user_name);
		close (fd);
		return;
	}
	if (uflg) {
		if (fchown (fd, user_newid, (gid_t) - 1) < 0) {
			perror (_("failed to change mailbox owner"));
		}
#ifdef WITH_AUDIT
		else {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "changing mail file owner", user_newname,
				      user_newid, 1);
		}
#endif
	}

	close (fd);

	if (lflg) {
		snprintf (newmailfile, sizeof newmailfile, "%s/%s",
			  maildir, user_newname);
		if (link (mailfile, newmailfile) || unlink (mailfile)) {
			perror (_("failed to rename mailbox"));
		}
#ifdef WITH_AUDIT
		else {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "changing mail file name", user_newname,
				      user_newid, 1);
		}
#endif
	}
}
#endif

/*
 * main - usermod command
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

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	user_groups = malloc ((1 + sys_ngroups) * sizeof (char *));
	user_groups[0] = (char *) 0;

	OPENLOG ("usermod");

	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

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
			retval = pam_start ("usermod", pampw->pw_name,
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

	/*
	 * Do the hard stuff - open the files, change the user entries,
	 * change the home directory, then close and update the files.
	 */
	open_files ();
	usr_update ();
	if (Gflg || lflg)
		grp_update ();
	close_files ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	if (mflg)
		move_home ();

#ifndef NO_MOVE_MAILBOX
	if (lflg || uflg)
		move_mailbox ();
#endif

	if (uflg) {
		update_files ();

		/*
		 * Change the UID on all of the files owned by `user_id' to
		 * `user_newid' in the user's home directory.
		 */
		chown_tree (dflg ? user_newhome : user_home,
			    user_id, user_newid,
			    user_gid, gflg ? user_newgid : user_gid);
	}

#ifdef USE_PAM
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	exit (E_SUCCESS);
	/* NOT REACHED */
}
