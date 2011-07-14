/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2011, Nicolas François
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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <lastlog.h>
#include <pwd.h>
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
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
#ifdef WITH_TCB
#include "tcbfuncs.h"
#endif

/*
 * exit status values
 * for E_GRP_UPDATE and E_NOSPACE (not used yet), other update requests
 * will be implemented (as documented in the Solaris 2.x man page).
 */
/*@-exitarg@*/
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
const char *Prog;

static char *user_name;
static char *user_newname = NULL;
static char *user_pass;
static uid_t user_id;
static uid_t user_newid;
static gid_t user_gid;
static gid_t user_newgid;
static char *user_comment;
static char *user_newcomment = NULL;
static char *user_home;
static char *user_newhome = NULL;
static char *user_shell;
#ifdef WITH_SELINUX
static const char *user_selinux = "";
#endif
static char *user_newshell = NULL;
static long user_expire;
static long user_newexpire;
static long user_inactive;
static long user_newinactive;
static long sys_ngroups;
static char **user_groups;	/* NULL-terminated list */

static bool
    aflg = false,		/* append to existing secondary group set */
    cflg = false,		/* new comment (GECOS) field */
    dflg = false,		/* new home directory */
    eflg = false,		/* days since 1970-01-01 when account becomes expired */
    fflg = false,		/* days until account with expired password is locked */
    gflg = false,		/* new primary group ID */
    Gflg = false,		/* new secondary group set */
    Lflg = false,		/* lock the password */
    lflg = false,		/* new user name */
    mflg = false,		/* create user's home directory if it doesn't exist */
    oflg = false,		/* permit non-unique user ID to be specified with -u */
    pflg = false,		/* new encrypted password */
    sflg = false,		/* new shell program */
#ifdef WITH_SELINUX
    Zflg = false,		/* new selinux user */
#endif
    uflg = false,		/* specify new user ID */
    Uflg = false;		/* unlock the password */

static bool is_shadow_pwd;

#ifdef SHADOWGRP
static bool is_shadow_grp;
#endif

static bool pw_locked  = false;
static bool spw_locked = false;
static bool gr_locked  = false;
#ifdef SHADOWGRP
static bool sgr_locked = false;
#endif


/* local function prototypes */
static void date_to_str (char *buf, size_t maxsize,
                         long int date, const char *negativ);
static int get_groups (char *);
static /*@noreturn@*/void usage (int status);
static void new_pwent (struct passwd *);
#ifdef WITH_SELINUX
static void selinux_update_mapping (void);
#endif

static void new_spent (struct spwd *);
static void fail_exit (int);
static void update_group (void);

#ifdef SHADOWGRP
static void update_gshadow (void);
#endif
static void grp_update (void);

static void process_flags (int, char **);
static void close_files (void);
static void open_files (void);
static void usr_update (void);
static void move_home (void);
static void update_lastlog (void);
static void update_faillog (void);

#ifndef NO_MOVE_MAILBOX
static void move_mailbox (void);
#endif

static void date_to_str (char *buf, size_t maxsize,
                         long int date, const char *negativ)
{
	struct tm *tp;

	if ((negativ != NULL) && (date < 0)) {
		strncpy (buf, negativ, maxsize);
	} else {
		time_t t = (time_t) date;
		tp = gmtime (&t);
#ifdef HAVE_STRFTIME
		strftime (buf, maxsize, "%Y-%m-%d", tp);
#else
		snprintf (buf, maxsize, "%04d-%02d-%02d",
		          tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
#endif				/* HAVE_STRFTIME */
	}
	buf[maxsize - 1] = '\0';
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

	if ('\0' == *list) {
		return 0;
	}

	/*
	 * So long as there is some data to be converted, strip off each
	 * name and look it up. A mix of numerical and string values for
	 * group identifiers is permitted.
	 */
	do {
		/*
		 * Strip off a single name from the list
		 */
		cp = strchr (list, ',');
		if (NULL != cp) {
			*cp = '\0';
			cp++;
		}

		/*
		 * Names starting with digits are treated as numerical GID
		 * values, otherwise the string is looked up as is.
		 */
		grp = getgr_nam_gid (list);

		/*
		 * There must be a match, either by GID value or by
		 * string name.
		 */
		if (NULL == grp) {
			fprintf (stderr, _("%s: group '%s' does not exist\n"),
			         Prog, list);
			errors++;
		}
		list = cp;

		/*
		 * If the group doesn't exist, don't dump core. Instead,
		 * try the next one.  --marekm
		 */
		if (NULL == grp) {
			continue;
		}

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
			         _("%s: too many groups specified (max %d).\n"),
			         Prog, ngroups);
			break;
		}

		/*
		 * Add the group name to the user's list of groups.
		 */
		user_groups[ngroups++] = xstrdup (grp->gr_name);
	} while (NULL != list);

	user_groups[ngroups] = (char *) 0;

	/*
	 * Any errors in finding group names are fatal
	 */
	if (0 != errors) {
		return -1;
	}

	return 0;
}

/*
 * usage - display usage message and exit
 */
static /*@noreturn@*/void usage (int status)
{
	fprintf ((E_SUCCESS != status) ? stderr : stdout,
	         _("Usage: usermod [options] LOGIN\n"
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
	         "%s"
	         "\n"),
#ifdef WITH_SELINUX
	         _("  -Z, --selinux-user            new SELinux user mapping for the user account\n")
#else
	         ""
#endif
	         );
	exit (status);
}

/*
 * update encrypted password string (for both shadow and non-shadow
 * passwords)
 */
static char *new_pw_passwd (char *pw_pass)
{
	if (Lflg && ('!' != pw_pass[0])) {
		char *buf = xmalloc (strlen (pw_pass) + 2);

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "updating passwd",
		              user_newname, (unsigned int) user_newid, 0);
#endif
		SYSLOG ((LOG_INFO, "lock user '%s' password", user_newname));
		strcpy (buf, "!");
		strcat (buf, pw_pass);
		pw_pass = buf;
	} else if (Uflg && pw_pass[0] == '!') {
		char *s;

		if (pw_pass[1] == '\0') {
			fprintf (stderr,
			         _("%s: unlocking the user's password would result in a passwordless account.\n"
			           "You should set a password with usermod -p to unlock this user's password.\n"),
			         Prog);
			return pw_pass;
		}

#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "updating password",
		              user_newname, (unsigned int) user_newid, 0);
#endif
		SYSLOG ((LOG_INFO, "unlock user '%s' password", user_newname));
		s = pw_pass;
		while ('\0' != *s) {
			*s = *(s + 1);
			s++;
		}
	} else if (pflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing password",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO, "change user '%s' password", user_newname));
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
		if (pw_locate (user_newname) != NULL) {
			fprintf (stderr,
			         _("%s: user '%s' already exists in %s\n"),
			         Prog, user_newname, pw_dbname ());
			fail_exit (E_NAME_IN_USE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing name",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user name '%s' to '%s'",
		         pwent->pw_name, user_newname));
		pwent->pw_name = xstrdup (user_newname);
	}
	if (!is_shadow_pwd) {
		pwent->pw_passwd = new_pw_passwd (pwent->pw_passwd);
	}

	if (uflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing uid",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' UID from '%d' to '%d'",
		         pwent->pw_name, pwent->pw_uid, user_newid));
		pwent->pw_uid = user_newid;
	}
	if (gflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing primary group",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' GID from '%d' to '%d'",
		         pwent->pw_name, pwent->pw_gid, user_newgid));
		pwent->pw_gid = user_newgid;
	}
	if (cflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing comment",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		pwent->pw_gecos = user_newcomment;
	}

	if (dflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing home directory",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' home from '%s' to '%s'",
		         pwent->pw_name, pwent->pw_dir, user_newhome));
		pwent->pw_dir = user_newhome;
	}
	if (sflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing user shell",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' shell from '%s' to '%s'",
		         pwent->pw_name, pwent->pw_shell, user_newshell));
		pwent->pw_shell = user_newshell;
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
	if (lflg) {
		if (spw_locate (user_newname) != NULL) {
			fprintf (stderr,
			         _("%s: user '%s' already exists in %s\n"),
			         Prog, user_newname, spw_dbname ());
			fail_exit (E_NAME_IN_USE);
		}
		spent->sp_namp = xstrdup (user_newname);
	}

	if (fflg) {
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing inactive days",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' inactive from '%ld' to '%ld'",
		         spent->sp_namp, spent->sp_inact, user_newinactive));
		spent->sp_inact = user_newinactive;
	}
	if (eflg) {
		/* log dates rather than numbers of days. */
		char new_exp[16], old_exp[16];
		date_to_str (new_exp, sizeof(new_exp),
		             user_newexpire * DAY, "never");
		date_to_str (old_exp, sizeof(old_exp),
		             user_expire * DAY, "never");
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "changing expiration date",
		              user_newname, (unsigned int) user_newid, 1);
#endif
		SYSLOG ((LOG_INFO,
		         "change user '%s' expiration from '%s' to '%s'",
		         spent->sp_namp, old_exp, new_exp));
		spent->sp_expire = user_newexpire;
	}
	spent->sp_pwdp = new_pw_passwd (spent->sp_pwdp);
	if (pflg) {
		spent->sp_lstchg = (long) time ((time_t *) 0) / SCALE;
		if (0 == spent->sp_lstchg) {
			/* Better disable aging than requiring a password
			 * change */
			spent->sp_lstchg = -1;
		}
	}
}

/*
 * fail_exit - exit with an error code after unlocking files
 */
static void fail_exit (int code)
{
	if (gr_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
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
	if (spw_locked) {
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
	}
	if (pw_locked) {
		if (pw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
			/* continue */
		}
	}

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
	              "modifying account",
	              user_name, AUDIT_NO_ID, 0);
#endif
	exit (code);
}


static void update_group (void)
{
	bool is_member;
	bool was_member;
	bool changed;
	const struct group *grp;
	struct group *ngrp;

	changed = false;

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */
	while ((grp = gr_next ()) != NULL) {
		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		was_member = is_on_list (grp->gr_mem, user_name);
		is_member = Gflg && (   (was_member && aflg)
		                     || is_on_list (user_groups, grp->gr_name));

		if (!was_member && !is_member) {
			continue;
		}

		ngrp = __gr_dup (grp);
		if (NULL == ngrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, gr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}

		if (was_member) {
			if ((!Gflg) || is_member) {
				/* User was a member and is still a member
				 * of this group.
				 * But the user might have been renamed.
				 */
				if (lflg) {
					ngrp->gr_mem = del_list (ngrp->gr_mem,
					                         user_name);
					ngrp->gr_mem = add_list (ngrp->gr_mem,
					                         user_newname);
					changed = true;
#ifdef WITH_AUDIT
					audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					              "changing group member",
					              user_newname, AUDIT_NO_ID, 1);
#endif
					SYSLOG ((LOG_INFO,
					         "change '%s' to '%s' in group '%s'",
					         user_name, user_newname,
					         ngrp->gr_name));
				}
			} else {
				/* User was a member but is no more a
				 * member of this group.
				 */
				ngrp->gr_mem = del_list (ngrp->gr_mem, user_name);
				changed = true;
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				              "removing group member",
				              user_name, AUDIT_NO_ID, 1);
#endif
				SYSLOG ((LOG_INFO,
				         "delete '%s' from group '%s'",
				         user_name, ngrp->gr_name));
			}
		} else {
			/* User was not a member but is now a member this
			 * group.
			 */
			ngrp->gr_mem = add_list (ngrp->gr_mem, user_newname);
			changed = true;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "adding user to group",
			              user_name, AUDIT_NO_ID, 1);
#endif
			SYSLOG ((LOG_INFO, "add '%s' to group '%s'",
			         user_newname, ngrp->gr_name));
		}
		if (!changed) {
			continue;
		}

		changed = false;
		if (gr_update (ngrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, gr_dbname (), ngrp->gr_name);
			SYSLOG ((LOG_WARN, "failed to prepare the new %s entry '%s'", gr_dbname (), ngrp->gr_name));
			fail_exit (E_GRP_UPDATE);
		}
	}
}

#ifdef SHADOWGRP
static void update_gshadow (void)
{
	bool is_member;
	bool was_member;
	bool was_admin;
	bool changed;
	const struct sgrp *sgrp;
	struct sgrp *nsgrp;

	changed = false;

	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of.
	 */
	while ((sgrp = sgr_next ()) != NULL) {

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
		is_member = Gflg && (   (was_member && aflg)
		                     || is_on_list (user_groups, sgrp->sg_name));

		if (!was_member && !was_admin && !is_member) {
			continue;
		}

		nsgrp = __sgr_dup (sgrp);
		if (NULL == nsgrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, sgr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}

		if (was_admin && lflg) {
			/* User was an admin of this group but the user
			 * has been renamed.
			 */
			nsgrp->sg_adm = del_list (nsgrp->sg_adm, user_name);
			nsgrp->sg_adm = add_list (nsgrp->sg_adm, user_newname);
			changed = true;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "changing admin name in shadow group",
			              user_name, AUDIT_NO_ID, 1);
#endif
			SYSLOG ((LOG_INFO,
			         "change admin '%s' to '%s' in shadow group '%s'",
			         user_name, user_newname, nsgrp->sg_name));
		}

		if (was_member) {
			if ((!Gflg) || is_member) {
				/* User was a member and is still a member
				 * of this group.
				 * But the user might have been renamed.
				 */
				if (lflg) {
					nsgrp->sg_mem = del_list (nsgrp->sg_mem,
					                          user_name);
					nsgrp->sg_mem = add_list (nsgrp->sg_mem,
					                          user_newname);
					changed = true;
#ifdef WITH_AUDIT
					audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
					              "changing member in shadow group",
					              user_name, AUDIT_NO_ID, 1);
#endif
					SYSLOG ((LOG_INFO,
					         "change '%s' to '%s' in shadow group '%s'",
					         user_name, user_newname,
					         nsgrp->sg_name));
				}
			} else {
				/* User was a member but is no more a
				 * member of this group.
				 */
				nsgrp->sg_mem = del_list (nsgrp->sg_mem, user_name);
				changed = true;
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				              "removing user from shadow group",
				              user_name, AUDIT_NO_ID, 1);
#endif
				SYSLOG ((LOG_INFO,
				         "delete '%s' from shadow group '%s'",
				         user_name, nsgrp->sg_name));
			}
		} else if (is_member) {
			/* User was not a member but is now a member this
			 * group.
			 */
			nsgrp->sg_mem = add_list (nsgrp->sg_mem, user_newname);
			changed = true;
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "adding user to shadow group",
			              user_newname, AUDIT_NO_ID, 1);
#endif
			SYSLOG ((LOG_INFO, "add '%s' to shadow group '%s'",
			         user_newname, nsgrp->sg_name));
		}
		if (!changed) {
			continue;
		}

		changed = false;

		/* 
		 * Update the group entry to reflect the changes.
		 */
		if (sgr_update (nsgrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, sgr_dbname (), nsgrp->sg_name);
			SYSLOG ((LOG_WARN, "failed to prepare the new %s entry '%s'",
			         sgr_dbname (), nsgrp->sg_name));
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
	if (is_shadow_grp) {
		update_gshadow ();
	}
#endif
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

	bool anyflag = false;

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
#ifdef WITH_SELINUX
			{"selinux-user", required_argument, NULL, 'Z'},
#endif
			{"shell", required_argument, NULL, 's'},
			{"uid", required_argument, NULL, 'u'},
			{"unlock", no_argument, NULL, 'U'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c = getopt_long (argc, argv,
#ifdef WITH_SELINUX
			                 "ac:d:e:f:g:G:hl:Lmop:s:u:UZ:",
#else
			                 "ac:d:e:f:g:G:hl:Lmop:s:u:U",
#endif
			                 long_options, NULL)) != -1) {
			switch (c) {
			case 'a':
				aflg = true;
				break;
			case 'c':
				if (!VALID (optarg)) {
					fprintf (stderr,
					         _("%s: invalid field '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_newcomment = optarg;
				cflg = true;
				break;
			case 'd':
				if (!VALID (optarg)) {
					fprintf (stderr,
					         _("%s: invalid field '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				dflg = true;
				user_newhome = optarg;
				break;
			case 'e':
				if ('\0' != *optarg) {
					user_newexpire = strtoday (optarg);
					if (user_newexpire < -1) {
						fprintf (stderr,
						         _("%s: invalid date '%s'\n"),
						         Prog, optarg);
						exit (E_BAD_ARG);
					}
					user_newexpire *= DAY / SCALE;
				} else {
					user_newexpire = -1;
				}
				eflg = true;
				break;
			case 'f':
				if (   (getlong (optarg, &user_newinactive) == 0)
				    || (user_newinactive < -1)) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					usage (E_USAGE);
				}
				fflg = true;
				break;
			case 'g':
				grp = getgr_nam_gid (optarg);
				if (NULL == grp) {
					fprintf (stderr,
					         _("%s: group '%s' does not exist\n"),
					         Prog, optarg);
					exit (E_NOTFOUND);
				}
				user_newgid = grp->gr_gid;
				gflg = true;
				break;
			case 'G':
				if (get_groups (optarg) != 0) {
					exit (E_NOTFOUND);
				}
				Gflg = true;
				break;
			case 'h':
				usage (E_SUCCESS);
				/* @notreached@ */break;
			case 'l':
				if (!is_valid_user_name (optarg)) {
					fprintf (stderr,
					         _("%s: invalid field '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				lflg = true;
				user_newname = optarg;
				break;
			case 'L':
				Lflg = true;
				break;
			case 'm':
				mflg = true;
				break;
			case 'o':
				oflg = true;
				break;
			case 'p':
				user_pass = optarg;
				pflg = true;
				break;
			case 's':
				if (!VALID (optarg)) {
					fprintf (stderr,
					         _("%s: invalid field '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_newshell = optarg;
				sflg = true;
				break;
			case 'u':
				if (   (get_uid (optarg, &user_newid) ==0)
				    || (user_newid == (uid_t)-1)) {
					fprintf (stderr,
					         _("%s: invalid user ID '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				uflg = true;
				break;
			case 'U':
				Uflg = true;
				break;
#ifdef WITH_SELINUX
			case 'Z':
				if (is_selinux_enabled () > 0) {
					user_selinux = optarg;
					Zflg = true;
				} else {
					fprintf (stderr,
					         _("%s: -Z requires SELinux enabled kernel\n"),
					         Prog);
					exit (E_BAD_ARG);
				}
				break;
#endif
			default:
				usage (E_USAGE);
			}
			anyflag = true;
		}
	}

	if (optind != argc - 1) {
		usage (E_USAGE);
	}

	user_name = argv[argc - 1];

	{
		const struct passwd *pwd;
		/* local, no need for xgetpwnam */
		pwd = getpwnam (user_name);
		if (NULL == pwd) {
			fprintf (stderr,
			         _("%s: user '%s' does not exist\n"),
			         Prog, user_name);
			exit (E_NOTFOUND);
		}

		user_id = pwd->pw_uid;
		user_gid = pwd->pw_gid;
		user_comment = xstrdup (pwd->pw_gecos);
		user_home = xstrdup (pwd->pw_dir);
		user_shell = xstrdup (pwd->pw_shell);
	}

	/* user_newname, user_newid, user_newgid can be used even when the
	 * options where not specified. */
	if (!lflg) {
		user_newname = user_name;
	}
	if (!uflg) {
		user_newid = user_id;
	}
	if (!gflg) {
		user_newgid = user_gid;
	}

#ifdef	USE_NIS
	/*
	 * Now make sure it isn't an NIS user.
	 */
	if (__ispwNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr,
		         _("%s: user %s is a NIS user\n"),
		         Prog, user_name);

		if (   !yp_get_default_domain (&nis_domain)
		    && !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr,
			         _("%s: %s is the NIS master\n"),
			         Prog, nis_master);
		}
		exit (E_NOTFOUND);
	}
#endif

	{
		const struct spwd *spwd = NULL;
		/* local, no need for xgetspnam */
		if (is_shadow_pwd && ((spwd = getspnam (user_name)) != NULL)) {
			user_expire = spwd->sp_expire;
			user_inactive = spwd->sp_inact;
		}
	}

	if (!anyflag) {
		fprintf (stderr, _("%s: no options\n"), Prog);
		usage (E_USAGE);
	}

	if (aflg && (!Gflg)) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-a", "-G");
		usage (E_USAGE);
	}

	if ((Lflg && (pflg || Uflg)) || (pflg && Uflg)) {
		fprintf (stderr,
		         _("%s: the -L, -p, and -U flags are exclusive\n"),
		         Prog);
		usage (E_USAGE);
	}

	if (oflg && !uflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-o", "-u");
		usage (E_USAGE);
	}

	if (mflg && !dflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-m", "-d");
		usage (E_USAGE);
	}

	if (user_newid == user_id) {
		uflg = false;
		oflg = false;
	}
	if (user_newgid == user_gid) {
		gflg = false;
	}
	if (   (NULL != user_newshell)
	    && (strcmp (user_newshell, user_shell) == 0)) {
		sflg = false;
	}
	if (strcmp (user_newname, user_name) == 0) {
		lflg = false;
	}
	if (user_newinactive == user_inactive) {
		fflg = false;
	}
	if (user_newexpire == user_expire) {
		eflg = false;
	}
	if (   (NULL != user_newhome)
	    && (strcmp (user_newhome, user_home) == 0)) {
		dflg = false;
		mflg = false;
	}
	if (   (NULL != user_newcomment)
	    && (strcmp (user_newcomment, user_comment) == 0)) {
		cflg = false;
	}

	if (!(Uflg || uflg || sflg || pflg || mflg || Lflg ||
	      lflg || Gflg || gflg || fflg || eflg || dflg || cflg
#ifdef WITH_SELINUX
	      || Zflg
#endif
	)) {
		fprintf (stderr, _("%s: no changes\n"), Prog);
		exit (E_SUCCESS);
	}

	if (!is_shadow_pwd && (eflg || fflg)) {
		fprintf (stderr,
		         _("%s: shadow passwords required for -e and -f\n"),
		         Prog);
		exit (E_USAGE);
	}

	/* local, no need for xgetpwnam */
	if (lflg && (getpwnam (user_newname) != NULL)) {
		fprintf (stderr,
		         _("%s: user '%s' already exists\n"),
		         Prog, user_newname);
		exit (E_NAME_IN_USE);
	}

	/* local, no need for xgetpwuid */
	if (uflg && !oflg && (getpwuid (user_newid) != NULL)) {
		fprintf (stderr,
		         _("%s: UID '%lu' already exists\n"),
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
	if (pw_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", pw_dbname ()));
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && (spw_close () == 0)) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, spw_dbname ());
		SYSLOG ((LOG_ERR,
		         "failure while writing changes to %s",
		         spw_dbname ()));
		fail_exit (E_PW_UPDATE);
	}

	if (Gflg || lflg) {
		if (gr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, gr_dbname ());
			SYSLOG ((LOG_ERR,
			         "failure while writing changes to %s",
			         gr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
#ifdef SHADOWGRP
		if (is_shadow_grp) {
			if (sgr_close () == 0) {
				fprintf (stderr,
				         _("%s: failure while writing changes to %s\n"),
				         Prog, sgr_dbname ());
				SYSLOG ((LOG_ERR,
				         "failure while writing changes to %s",
				         sgr_dbname ()));
				fail_exit (E_GRP_UPDATE);
			}
			if (sgr_unlock () == 0) {
				fprintf (stderr,
				         _("%s: failed to unlock %s\n"),
				         Prog, sgr_dbname ());
				SYSLOG ((LOG_ERR,
				         "failed to unlock %s",
				         sgr_dbname ()));
				/* continue */
			}
		}
#endif
		if (gr_unlock () == 0) {
			fprintf (stderr,
			         _("%s: failed to unlock %s\n"),
			         Prog, gr_dbname ());
			SYSLOG ((LOG_ERR,
			         "failed to unlock %s",
			         gr_dbname ()));
			/* continue */
		}
	}

	if (is_shadow_pwd) {
		if (spw_unlock () == 0) {
			fprintf (stderr,
			         _("%s: failed to unlock %s\n"),
			         Prog, spw_dbname ());
			SYSLOG ((LOG_ERR,
			         "failed to unlock %s",
			         spw_dbname ()));
			/* continue */
		}
	}
	if (pw_unlock () == 0) {
		fprintf (stderr,
		         _("%s: failed to unlock %s\n"),
		         Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
		/* continue */
	}

	pw_locked = false;
	spw_locked = false;
	gr_locked = false;
#ifdef	SHADOWGRP
	sgr_locked = false;
#endif

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
	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
	pw_locked = true;
	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"),
		         Prog, pw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd && (spw_lock () == 0)) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
	spw_locked = true;
	if (is_shadow_pwd && (spw_open (O_RDWR) == 0)) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}

	if (Gflg || lflg) {
		/*
		 * Lock and open the group file. This will load all of the
		 * group entries.
		 */
		if (gr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, gr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
		gr_locked = true;
		if (gr_open (O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, gr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
#ifdef SHADOWGRP
		if (is_shadow_grp && (sgr_lock () == 0)) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			fail_exit (E_GRP_UPDATE);
		}
		sgr_locked = true;
		if (is_shadow_grp && (sgr_open (O_RDWR) == 0)) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
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
	if (NULL == pwd) {
		fprintf (stderr,
		         _("%s: user '%s' does not exist in %s\n"),
		         Prog, user_name, pw_dbname ());
		fail_exit (E_NOTFOUND);
	}
	pwent = *pwd;
	new_pwent (&pwent);


	/* 
	 * Locate the entry in /etc/shadow. It doesn't have to exist, and
	 * won't be created if it doesn't.
	 */
	if (is_shadow_pwd && ((spwd = spw_locate (user_name)) != NULL)) {
		spent = *spwd;
		new_spent (&spent);
	}

	if (lflg || uflg || gflg || cflg || dflg || sflg || pflg
	    || Lflg || Uflg) {
		if (pw_update (&pwent) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, pw_dbname (), pwent.pw_name);
			fail_exit (E_PW_UPDATE);
		}
		if (lflg && (pw_remove (user_name) == 0)) {
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, user_name, pw_dbname ());
			fail_exit (E_PW_UPDATE);
		}
	}
	if ((NULL != spwd) && (lflg || eflg || fflg || pflg || Lflg || Uflg)) {
		if (spw_update (&spent) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, spw_dbname (), spent.sp_namp);
			fail_exit (E_PW_UPDATE);
		}
		if (lflg && (spw_remove (user_name) == 0)) {
			fprintf (stderr,
			         _("%s: cannot remove entry '%s' from %s\n"),
			         Prog, user_name, spw_dbname ());
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

	if (access (user_newhome, F_OK) == 0) {
		/*
		 * If the new home directory already exist, the user
		 * should not use -m.
		 */
		fprintf (stderr,
		         _("%s: directory %s exists\n"),
		         Prog, user_newhome);
		fail_exit (E_HOMEDIR);
	}

	if (stat (user_home, &sb) == 0) {
		/*
		 * Don't try to move it if it is not a directory
		 * (but /dev/null for example).  --marekm
		 */
		if (!S_ISDIR (sb.st_mode)) {
			fprintf (stderr,
			         _("%s: The previous home directory (%s) was "
			           "not a directory. It is not removed and no "
			           "home directories are created.\n"),
			         Prog, user_home);
			fail_exit (E_HOMEDIR);
		}

		if (rename (user_home, user_newhome) == 0) {
			/* FIXME: rename above may have broken symlinks
			 *        pointing to the user's home directory
			 *        with an absolute path. */
			if (chown_tree (user_newhome,
			                user_id,  uflg ? user_newid  : (uid_t)-1,
			                user_gid, gflg ? user_newgid : (gid_t)-1) != 0) {
				fprintf (stderr,
				         _("%s: Failed to change ownership of the home directory"),
				         Prog);
				fail_exit (E_HOMEDIR);
			}
			return;
		} else {
			if (EXDEV == errno) {
				if (copy_tree (user_home, user_newhome, true,
				               true,
				               user_id,
				               uflg ? user_newid : (uid_t)-1,
				               user_gid,
				               gflg ? user_newgid : (gid_t)-1) == 0) {
					if (remove_tree (user_home, true) != 0) {
						fprintf (stderr,
						         _("%s: warning: failed to completely remove old home directory %s"),
						         Prog, user_home);
					}
#ifdef WITH_AUDIT
					audit_logger (AUDIT_USER_CHAUTHTOK,
					              Prog,
					              "moving home directory",
					              user_newname,
					              (unsigned int) user_newid,
					              1);
#endif
					return;
				}

				(void) remove_tree (user_newhome, true);
			}
			fprintf (stderr,
			         _("%s: cannot rename directory %s to %s\n"),
			         Prog, user_home, user_newhome);
			fail_exit (E_HOMEDIR);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
		              "moving home directory",
		              user_newname, (unsigned int) user_newid, 1);
#endif
	}
}

/*
 * update_lastlog - update the lastlog file
 *
 * Relocate the "lastlog" entries for the user. The old entry is
 * left alone in case the UID was shared. It doesn't hurt anything
 * to just leave it be.
 */
static void update_lastlog (void)
{
	struct lastlog ll;
	int fd;
	off_t off_uid = (off_t) user_id * sizeof ll;
	off_t off_newuid = (off_t) user_newid * sizeof ll;

	if (access (LASTLOG_FILE, F_OK) != 0) {
		return;
	}

	fd = open (LASTLOG_FILE, O_RDWR);

	if (-1 == fd) {
		fprintf (stderr,
		         _("%s: failed to copy the lastlog entry of user %lu to user %lu: %s\n"),
		         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
		return;
	}

	if (   (lseek (fd, off_uid, SEEK_SET) == off_uid)
	    && (read (fd, &ll, sizeof ll) == (ssize_t) sizeof ll)) {
		/* Copy the old entry to its new location */
		if (   (lseek (fd, off_newuid, SEEK_SET) != off_newuid)
		    || (write (fd, &ll, sizeof ll) != (ssize_t) sizeof ll)
		    || (fsync (fd) != 0)
		    || (close (fd) != 0)) {
			fprintf (stderr,
			         _("%s: failed to copy the lastlog entry of user %lu to user %lu: %s\n"),
			         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
		}
	} else {
		/* Assume lseek or read failed because there is
		 * no entry for the old UID */

		/* Check if the new UID already has an entry */
		if (   (lseek (fd, off_newuid, SEEK_SET) == off_newuid)
		    && (read (fd, &ll, sizeof ll) == (ssize_t) sizeof ll)) {
			/* Reset the new uid's lastlog entry */
			memzero (&ll, sizeof (ll));
			if (   (lseek (fd, off_newuid, SEEK_SET) != off_newuid)
			    || (write (fd, &ll, sizeof ll) != (ssize_t) sizeof ll)
			    || (fsync (fd) != 0)
			    || (close (fd) != 0)) {
				fprintf (stderr,
				         _("%s: failed to copy the lastlog entry of user %lu to user %lu: %s\n"),
				         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
			}
		} else {
			(void) close (fd);
		}
	}
}

/*
 * update_faillog - update the faillog file
 *
 * Relocate the "faillog" entries for the user. The old entry is
 * left alone in case the UID was shared. It doesn't hurt anything
 * to just leave it be.
 */
static void update_faillog (void)
{
	struct faillog fl;
	int fd;
	off_t off_uid = (off_t) user_id * sizeof fl;
	off_t off_newuid = (off_t) user_newid * sizeof fl;

	if (access (FAILLOG_FILE, F_OK) != 0) {
		return;
	}

	fd = open (FAILLOG_FILE, O_RDWR);

	if (-1 == fd) {
		fprintf (stderr,
		         _("%s: failed to copy the faillog entry of user %lu to user %lu: %s\n"),
		         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
		return;
	}

	if (   (lseek (fd, off_uid, SEEK_SET) == off_uid)
	    && (read (fd, (char *) &fl, sizeof fl) == (ssize_t) sizeof fl)) {
		/* Copy the old entry to its new location */
		if (   (lseek (fd, off_newuid, SEEK_SET) != off_newuid)
		    || (write (fd, &fl, sizeof fl) != (ssize_t) sizeof fl)
		    || (fsync (fd) != 0)
		    || (close (fd) != 0)) {
			fprintf (stderr,
			         _("%s: failed to copy the faillog entry of user %lu to user %lu: %s\n"),
			         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
		}
	} else {
		/* Assume lseek or read failed because there is
		 * no entry for the old UID */

		/* Check if the new UID already has an entry */
		if (   (lseek (fd, off_newuid, SEEK_SET) == off_newuid)
		    && (read (fd, &fl, sizeof fl) == (ssize_t) sizeof fl)) {
			/* Reset the new uid's faillog entry */
			memzero (&fl, sizeof (fl));
			if (   (lseek (fd, off_newuid, SEEK_SET) != off_newuid)
			    || (write (fd, &fl, sizeof fl) != (ssize_t) sizeof fl)
			    || (close (fd) != 0)) {
				fprintf (stderr,
				         _("%s: failed to copy the faillog entry of user %lu to user %lu: %s\n"),
				         Prog, (unsigned long) user_id, (unsigned long) user_newid, strerror (errno));
			}
		} else {
			(void) close (fd);
		}
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
	if ((NULL == maildir) && (getdef_str ("MAIL_FILE") == NULL)) {
		maildir = MAIL_SPOOL_DIR;
	}
#endif
	if (NULL == maildir) {
		return;
	}

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
		if (errno != ENOENT) {
			perror (mailfile);
		}
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
		if (fchown (fd, user_newid, (gid_t) -1) < 0) {
			perror (_("failed to change mailbox owner"));
		}
#ifdef WITH_AUDIT
		else {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "changing mail file owner",
			              user_newname, (unsigned int) user_newid, 1);
		}
#endif
	}

	close (fd);

	if (lflg) {
		snprintf (newmailfile, sizeof newmailfile, "%s/%s",
		          maildir, user_newname);
		if (   (link (mailfile, newmailfile) != 0)
		    || (unlink (mailfile) != 0)) {
			perror (_("failed to rename mailbox"));
		}
#ifdef WITH_AUDIT
		else {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "changing mail file name",
			              user_newname, (unsigned int) user_newid, 1);
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
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

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

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	user_groups = (char **) malloc (sizeof (char *) * (1 + sys_ngroups));
	user_groups[0] = (char *) 0;

	OPENLOG ("usermod");

	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	process_flags (argc, argv);

	/*
	 * The home directory, the username and the user's UID should not
	 * be changed while the user is logged in.
	 */
	if (   (uflg || lflg || dflg)
	    && (user_busy (user_name, user_id) != 0)) {
		exit (E_USER_BUSY);
	}

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			exit (1);
		}

		retval = pam_start ("usermod", pampw->pw_name, &conv, &pamh);
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

#ifdef WITH_TCB
	if (shadowtcb_set_user (user_name) == SHADOWTCB_FAILURE) {
		exit (E_PW_UPDATE);
	}
#endif

	/*
	 * Do the hard stuff - open the files, change the user entries,
	 * change the home directory, then close and update the files.
	 */
	open_files ();
	if (   cflg || dflg || eflg || fflg || gflg || Lflg || lflg || pflg
	    || sflg || uflg || Uflg) {
		usr_update ();
	}
	if (Gflg || lflg) {
		grp_update ();
	}
	close_files ();

#ifdef WITH_TCB
	if (   (lflg || uflg)
	    && (shadowtcb_move (user_newname, user_newid) == SHADOWTCB_FAILURE) ) {
		exit (E_PW_UPDATE);
	}
#endif

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

#ifdef WITH_SELINUX
	if (Zflg) {
		selinux_update_mapping ();
	}
#endif

	if (mflg) {
		move_home ();
	}

#ifndef NO_MOVE_MAILBOX
	if (lflg || uflg) {
		move_mailbox ();
	}
#endif

	if (uflg) {
		update_lastlog ();
		update_faillog ();
	}

	if (!mflg && (uflg || gflg)) {
		if (access (dflg ? user_newhome : user_home, F_OK) == 0) {
			/*
			 * Change the UID on all of the files owned by
			 * `user_id' to `user_newid' in the user's home
			 * directory.
			 *
			 * move_home() already takes care of changing the
			 * ownership.
			 *
			 */
			if (chown_tree (dflg ? user_newhome : user_home,
			                user_id,
			                uflg ? user_newid  : (uid_t)-1,
			                user_gid,
			                gflg ? user_newgid : (gid_t)-1) != 0) {
				fprintf (stderr,
				         _("%s: Failed to change ownership of the home directory"),
				         Prog);
				fail_exit (E_HOMEDIR);
			}
		}
	}

	return E_SUCCESS;
}

#ifdef WITH_SELINUX
static void selinux_update_mapping (void) {
	const char *argv[7];

	if (is_selinux_enabled () <= 0) {
		return;
	}

	if ('\0' != *user_selinux) {
		argv[0] = "/usr/sbin/semanage";
		argv[1] = "login";
		argv[2] = "-m";
		argv[3] = "-s";
		argv[4] = user_selinux;
		argv[5] = user_name;
		argv[6] = NULL;
		if (safe_system (argv[0], argv, NULL, true) != 0) {
			argv[2] = "-a";
			if (safe_system (argv[0], argv, NULL, false) != 0) {
				fprintf (stderr,
				         _("%s: warning: the user name %s to %s SELinux user mapping failed.\n"),
				         Prog, user_name, user_selinux);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				              "modifying User mapping ",
				              user_name, (unsigned int) user_id, 0);
#endif
			}
		}
	}
}
#endif

