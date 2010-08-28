/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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
#include <string.h>
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

#ifndef SKEL_DIR
#define SKEL_DIR "/etc/skel"
#endif
#ifndef USER_DEFAULTS_FILE
#define USER_DEFAULTS_FILE "/etc/default/useradd"
#define NEW_USER_FILE "/etc/default/nuaddXXXXXX"
#endif
/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif
/*
 * Global variables
 */
const char *Prog;

/*
 * These defaults are used if there is no defaults file.
 */
static gid_t def_group = 100;
static const char *def_gname = "other";
static const char *def_home = "/home";
static const char *def_shell = "";
static const char *def_template = SKEL_DIR;
static const char *def_create_mail_spool = "no";

static long def_inactive = -1;
static const char *def_expire = "";

static char def_file[] = USER_DEFAULTS_FILE;

#define	VALID(s)	(strcspn (s, ":\n") == strlen (s))

static const char *user_name = "";
static const char *user_pass = "!";
static uid_t user_id;
static gid_t user_gid;
static const char *user_comment = "";
static const char *user_home = "";
static const char *user_shell = "";
static const char *create_mail_spool = "";
#ifdef WITH_SELINUX
static const char *user_selinux = "";
#endif

static long user_expire = -1;
static bool is_shadow_pwd;

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool pw_locked = false;
static bool gr_locked = false;
static bool spw_locked = false;
static char **user_groups;	/* NULL-terminated list */
static long sys_ngroups;
static bool do_grp_update = false;	/* group files need to be updated */

static bool
    bflg = false,		/* new default root of home directory */
    cflg = false,		/* comment (GECOS) field for new account */
    dflg = false,		/* home directory for new account */
    Dflg = false,		/* set/show new user default values */
    eflg = false,		/* days since 1970-01-01 when account is locked */
    fflg = false,		/* days until account with expired password is locked */
    gflg = false,		/* primary group ID for new account */
    Gflg = false,		/* secondary group set for new account */
    kflg = false,		/* specify a directory to fill new user directory */
    lflg = false,		/* do not add user to lastlog/faillog databases */
    mflg = false,		/* create user's home directory if it doesn't exist */
    Mflg = false,		/* do not create user's home directory even if CREATE_HOME is set */
    Nflg = false,		/* do not create a group having the same name as the user, but add the user to def_group (or the group specified with -g) */
    oflg = false,		/* permit non-unique user ID to be specified with -u */
    rflg = false,		/* create a system account */
    sflg = false,		/* shell program for new account */
    uflg = false,		/* specify user ID for new account */
    Uflg = false,		/* create a group having the same name as the user */
    Zflg = false;		/* new selinux user */

static bool home_added = false;

/*
 * exit status values
 */
/*@-exitarg@*/
#define E_SUCCESS	0	/* success */
#define E_PW_UPDATE	1	/* can't update password file */
#define E_USAGE		2	/* invalid command syntax */
#define E_BAD_ARG	3	/* invalid argument to option */
#define E_UID_IN_USE	4	/* UID already in use (and no -o) */
#define E_NOTFOUND	6	/* specified group doesn't exist */
#define E_NAME_IN_USE	9	/* username already in use */
#define E_GRP_UPDATE	10	/* can't update group file */
#define E_HOMEDIR	12	/* can't create home directory */
#define	E_MAIL_SPOOL	13	/* can't create mail spool */

#define DGROUP			"GROUP="
#define DHOME			"HOME="
#define DSHELL			"SHELL="
#define DINACT			"INACTIVE="
#define DEXPIRE			"EXPIRE="
#define DSKEL			"SKEL="
#define DCREATE_MAIL_SPOOL	"CREATE_MAIL_SPOOL="

/* local function prototypes */
static void fail_exit (int);
static void get_defaults (void);
static void show_defaults (void);
static int set_defaults (void);
static int get_groups (char *);
static void usage (int status);
static void new_pwent (struct passwd *);
#ifdef WITH_SELINUX
static void selinux_update_mapping (void);
#endif

static long scale_age (long);
static void new_spent (struct spwd *);
static void grp_update (void);

static void process_flags (int argc, char **argv);
static void close_files (void);
static void open_files (void);
static void open_shadow (void);
static void faillog_reset (uid_t);
static void lastlog_reset (uid_t);
static void usr_update (void);
static void create_home (void);
static void create_mail (void);

/*
 * fail_exit - undo as much as possible
 */
static void fail_exit (int code)
{
	if (home_added) {
		if (rmdir (user_home) != 0) {
			fprintf (stderr,
			         _("%s: %s was created, but could not be removed\n"),
			         Prog, user_home);
			SYSLOG ((LOG_ERR, "failed to remove %s", user_home));
		}
	}

	if (spw_locked) {
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking shadow file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
	}
	if (pw_locked) {
		if (pw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking passwd file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
	}
	if (gr_locked) {
		if (gr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking group file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
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
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking gshadow file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
	}
#endif

#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_USER, Prog,
	              "adding user",
	              user_name, AUDIT_NO_ID,
	              SHADOW_AUDIT_FAILURE);
#endif
	SYSLOG ((LOG_INFO, "failed adding user '%s', data deleted", user_name));
	exit (code);
}

#define MATCH(x,y) (strncmp((x),(y),strlen(y)) == 0)

/*
 * get_defaults - read the defaults file
 *
 *	get_defaults() reads the defaults file for this command. It sets the
 *	various values from the file, or uses built-in default values if the
 *	file does not exist.
 */
static void get_defaults (void)
{
	FILE *fp;
	char buf[1024];
	char *cp;

	/*
	 * Open the defaults file for reading.
	 */

	fp = fopen (def_file, "r");
	if (NULL == fp) {
		return;
	}

	/*
	 * Read the file a line at a time. Only the lines that have relevant
	 * values are used, everything else can be ignored.
	 */
	while (fgets (buf, (int) sizeof buf, fp) == buf) {
		cp = strrchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		}

		cp = strchr (buf, '=');
		if (NULL == cp) {
			continue;
		}

		cp++;

		/*
		 * Primary GROUP identifier
		 */
		if (MATCH (buf, DGROUP)) {
			const struct group *grp = getgr_nam_gid (cp);
			if (NULL == grp) {
				fprintf (stderr,
				         _("%s: group '%s' does not exist\n"),
				         Prog, cp);
				fprintf (stderr,
				         _("%s: the %s configuration in %s will be ignored\n"),
				         Prog, DGROUP, def_file);
			} else {
				def_group = grp->gr_gid;
				def_gname = xstrdup (grp->gr_name);
			}
		}

		/*
		 * Default HOME filesystem
		 */
		else if (MATCH (buf, DHOME)) {
			def_home = xstrdup (cp);
		}

		/*
		 * Default Login Shell command
		 */
		else if (MATCH (buf, DSHELL)) {
			def_shell = xstrdup (cp);
		}

		/*
		 * Default Password Inactive value
		 */
		else if (MATCH (buf, DINACT)) {
			if (   (getlong (cp, &def_inactive) == 0)
			    || (def_inactive < -1)) {
				fprintf (stderr,
				         _("%s: invalid numeric argument '%s'\n"),
				         Prog, optarg);
				fprintf (stderr,
				         _("%s: the %s configuration in %s will be ignored\n"),
				         Prog, DINACT, def_file);
				def_inactive = -1;
			}
		}

		/*
		 * Default account expiration date
		 */
		else if (MATCH (buf, DEXPIRE)) {
			def_expire = xstrdup (cp);
		}

		/*
		 * Default Skeleton information
		 */
		else if (MATCH (buf, DSKEL)) {
			if ('\0' == *cp) {
				cp = SKEL_DIR;	/* XXX warning: const */
			}

			def_template = xstrdup (cp);
		}

		/*
		 * Create by default user mail spool or not ?
		 */
		else if (MATCH (buf, DCREATE_MAIL_SPOOL)) {
			if (*cp == '\0') {
				cp = "no";	/* XXX warning: const */
			}

			def_create_mail_spool = xstrdup (cp);
		}
	}
	(void) fclose (fp);
}

/*
 * show_defaults - show the contents of the defaults file
 *
 *	show_defaults() displays the values that are used from the default
 *	file and the built-in values.
 */
static void show_defaults (void)
{
	printf ("GROUP=%u\n", (unsigned int) def_group);
	printf ("HOME=%s\n", def_home);
	printf ("INACTIVE=%ld\n", def_inactive);
	printf ("EXPIRE=%s\n", def_expire);
	printf ("SHELL=%s\n", def_shell);
	printf ("SKEL=%s\n", def_template);
	printf ("CREATE_MAIL_SPOOL=%s\n", def_create_mail_spool);
}

/*
 * set_defaults - write new defaults file
 *
 *	set_defaults() re-writes the defaults file using the values that
 *	are currently set. Duplicated lines are pruned, missing lines are
 *	added, and unrecognized lines are copied as is.
 */
static int set_defaults (void)
{
	FILE *ifp;
	FILE *ofp;
	char buf[1024];
	static char new_file[] = NEW_USER_FILE;
	char *cp;
	int ofd;
	int wlen;
	bool out_group = false;
	bool out_home = false;
	bool out_inactive = false;
	bool out_expire = false;
	bool out_shell = false;
	bool out_skel = false;
	bool out_create_mail_spool = false;

	/*
	 * Create a temporary file to copy the new output to.
	 */
	ofd = mkstemp (new_file);
	if (-1 == ofd) {
		fprintf (stderr,
		         _("%s: cannot create new defaults file\n"),
		         Prog);
		return -1;
	}

	ofp = fdopen (ofd, "w");
	if (NULL == ofp) {
		fprintf (stderr,
		         _("%s: cannot open new defaults file\n"),
		         Prog);
		return -1;
	}

	/*
	 * Open the existing defaults file and copy the lines to the
	 * temporary file, using any new values. Each line is checked
	 * to insure that it is not output more than once.
	 */
	ifp = fopen (def_file, "r");
	if (NULL == ifp) {
		fprintf (ofp, "# useradd defaults file\n");
		goto skip;
	}

	while (fgets (buf, (int) sizeof buf, ifp) == buf) {
		cp = strrchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		} else {
			/* A line which does not end with \n is only valid
			 * at the end of the file.
			 */
			if (feof (ifp) == 0) {
				fprintf (stderr,
				         _("%s: line too long in %s: %s..."),
				         Prog, def_file, buf);
				(void) fclose (ifp);
				return -1;
			}
		}

		if (!out_group && MATCH (buf, DGROUP)) {
			fprintf (ofp, DGROUP "%u\n", (unsigned int) def_group);
			out_group = true;
		} else if (!out_home && MATCH (buf, DHOME)) {
			fprintf (ofp, DHOME "%s\n", def_home);
			out_home = true;
		} else if (!out_inactive && MATCH (buf, DINACT)) {
			fprintf (ofp, DINACT "%ld\n", def_inactive);
			out_inactive = true;
		} else if (!out_expire && MATCH (buf, DEXPIRE)) {
			fprintf (ofp, DEXPIRE "%s\n", def_expire);
			out_expire = true;
		} else if (!out_shell && MATCH (buf, DSHELL)) {
			fprintf (ofp, DSHELL "%s\n", def_shell);
			out_shell = true;
		} else if (!out_skel && MATCH (buf, DSKEL)) {
			fprintf (ofp, DSKEL "%s\n", def_template);
			out_skel = true;
		} else if (!out_create_mail_spool
			   && MATCH (buf, DCREATE_MAIL_SPOOL)) {
			fprintf (ofp,
			         DCREATE_MAIL_SPOOL "%s\n",
			         def_create_mail_spool);
			out_create_mail_spool = true;
		} else
			fprintf (ofp, "%s\n", buf);
	}
	(void) fclose (ifp);

      skip:
	/*
	 * Check each line to insure that every line was output. This
	 * causes new values to be added to a file which did not previously
	 * have an entry for that value.
	 */
	if (!out_group)
		fprintf (ofp, DGROUP "%u\n", (unsigned int) def_group);
	if (!out_home)
		fprintf (ofp, DHOME "%s\n", def_home);
	if (!out_inactive)
		fprintf (ofp, DINACT "%ld\n", def_inactive);
	if (!out_expire)
		fprintf (ofp, DEXPIRE "%s\n", def_expire);
	if (!out_shell)
		fprintf (ofp, DSHELL "%s\n", def_shell);
	if (!out_skel)
		fprintf (ofp, DSKEL "%s\n", def_template);

	if (!out_create_mail_spool)
		fprintf (ofp, DCREATE_MAIL_SPOOL "%s\n", def_create_mail_spool);

	/*
	 * Flush and close the file. Check for errors to make certain
	 * the new file is intact.
	 */
	(void) fflush (ofp);
	if (   (ferror (ofp) != 0)
	    || (fsync (fileno (ofp)) != 0)
	    || (fclose (ofp) != 0)) {
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the current default file to its backup name.
	 */
	wlen = snprintf (buf, sizeof buf, "%s-", def_file);
	assert (wlen < (int) sizeof buf);
	if ((rename (def_file, buf) != 0) && (ENOENT != errno)) {
		int err = errno;
		fprintf (stderr,
		         _("%s: rename: %s: %s"),
		         Prog, def_file, strerror (err));
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the new default file to its correct name.
	 */
	if (rename (new_file, def_file) != 0) {
		int err = errno;
		fprintf (stderr,
		         _("%s: rename: %s: %s"),
		         Prog, new_file, strerror (err));
		return -1;
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USYS_CONFIG, Prog,
	              "changing useradd defaults",
	              NULL, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
	SYSLOG ((LOG_INFO,
	         "useradd defaults: GROUP=%u, HOME=%s, SHELL=%s, INACTIVE=%ld, "
	         "EXPIRE=%s, SKEL=%s, CREATE_MAIL_SPOOL=%s",
	         (unsigned int) def_group, def_home, def_shell,
	         def_inactive, def_expire, def_template,
	         def_create_mail_spool));
	return 0;
}

/*
 * get_groups - convert a list of group names to an array of group IDs
 *
 *	get_groups() takes a comma-separated list of group names and
 *	converts it to a NULL-terminated array. Any unknown group
 *	names are reported as errors.
 */
static int get_groups (char *list)
{
	char *cp;
	const struct group *grp;
	int errors = 0;
	int ngroups = 0;

	if ('\0' == *list) {
		return 0;
	}

	/*
	 * So long as there is some data to be converted, strip off
	 * each name and look it up. A mix of numerical and string
	 * values for group identifiers is permitted.
	 */
	do {
		/*
		 * Strip off a single name from the list
		 */
		cp = strchr (list, ',');
		if (NULL != cp) {
			*cp++ = '\0';
		}

		/*
		 * Names starting with digits are treated as numerical
		 * GID values, otherwise the string is looked up as is.
		 */
		grp = getgr_nam_gid (list);

		/*
		 * There must be a match, either by GID value or by
		 * string name.
		 */
		if (NULL == grp) {
			fprintf (stderr,
			         _("%s: group '%s' does not exist\n"),
			         Prog, list);
			errors++;
		}
		list = cp;

		/*
		 * If the group doesn't exist, don't dump core...
		 * Instead, try the next one.  --marekm
		 */
		if (NULL == grp) {
			continue;
		}

#ifdef	USE_NIS
		/*
		 * Don't add this group if they are an NIS group. Tell
		 * the user to go to the server for this group.
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
static void usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options] LOGIN\n"
	                  "\n"
	                  "Options:\n"),
	                Prog);
	(void) fputs (_("  -b, --base-dir BASE_DIR       base directory for the home directory of the\n"
	                "                                new account\n"), usageout);
	(void) fputs (_("  -c, --comment COMMENT         GECOS field of the new account\n"), usageout);
	(void) fputs (_("  -d, --home-dir HOME_DIR       home directory of the new account\n"), usageout);
	(void) fputs (_("  -D, --defaults                print or change default useradd configuration\n"), usageout);
	(void) fputs (_("  -e, --expiredate EXPIRE_DATE  expiration date of the new account\n"), usageout);
	(void) fputs (_("  -f, --inactive INACTIVE       password inactivity period of the new account\n"), usageout);
	(void) fputs (_("  -g, --gid GROUP               name or ID of the primary group of the new\n"
	                "                                account\n"), usageout);
	(void) fputs (_("  -G, --groups GROUPS           list of supplementary groups of the new\n"
	                "                                account\n"), usageout);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -k, --skel SKEL_DIR           use this alternative skeleton directory\n"), usageout);
	(void) fputs (_("  -K, --key KEY=VALUE           override /etc/login.defs defaults\n"), usageout);
	(void) fputs (_("  -l, --no-log-init             do not add the user to the lastlog and\n"
	                "                                faillog databases\n"), usageout);
	(void) fputs (_("  -m, --create-home             create the user's home directory\n"), usageout);
	(void) fputs (_("  -M, --no-create-home          do not create the user's home directory\n"), usageout);
	(void) fputs (_("  -N, --no-user-group           do not create a group with the same name as\n"
	                "                                the user\n"), usageout);
	(void) fputs (_("  -o, --non-unique              allow to create users with duplicate\n"
	                "                                (non-unique) UID\n"), usageout);
	(void) fputs (_("  -p, --password PASSWORD       encrypted password of the new account\n"), usageout);
	(void) fputs (_("  -r, --system                  create a system account\n"), usageout);
	(void) fputs (_("  -s, --shell SHELL             login shell of the new account\n"), usageout);
	(void) fputs (_("  -u, --uid UID                 user ID of the new account\n"), usageout);
	(void) fputs (_("  -U, --user-group              create a group with the same name as the user\n"), usageout);
#ifdef WITH_SELINUX
	(void) fputs (_("  -Z, --selinux-user SEUSER     use a specific SEUSER for the SELinux user mapping\n"), usageout);
#endif
	(void) fputs ("\n", usageout);
	exit (status);
}

/*
 * new_pwent - initialize the values in a password file entry
 *
 *	new_pwent() takes all of the values that have been entered and
 *	fills in a (struct passwd) with them.
 */
static void new_pwent (struct passwd *pwent)
{
	memzero (pwent, sizeof *pwent);
	pwent->pw_name = (char *) user_name;
	if (is_shadow_pwd) {
		pwent->pw_passwd = (char *) SHADOW_PASSWD_STRING;
	} else {
		pwent->pw_passwd = (char *) user_pass;
	}

	pwent->pw_uid = user_id;
	pwent->pw_gid = user_gid;
	pwent->pw_gecos = (char *) user_comment;
	pwent->pw_dir = (char *) user_home;
	pwent->pw_shell = (char *) user_shell;
}

static long scale_age (long x)
{
	if (x <= 0) {
		return x;
	}

	return x * (DAY / SCALE);
}

/*
 * new_spent - initialize the values in a shadow password file entry
 *
 *	new_spent() takes all of the values that have been entered and
 *	fills in a (struct spwd) with them.
 */
static void new_spent (struct spwd *spent)
{
	memzero (spent, sizeof *spent);
	spent->sp_namp = (char *) user_name;
	spent->sp_pwdp = (char *) user_pass;
	spent->sp_lstchg = (long) time ((time_t *) 0) / SCALE;
	if (0 == spent->sp_lstchg) {
		/* Better disable aging than requiring a password change */
		spent->sp_lstchg = -1;
	}
	if (!rflg) {
		spent->sp_min = scale_age (getdef_num ("PASS_MIN_DAYS", -1));
		spent->sp_max = scale_age (getdef_num ("PASS_MAX_DAYS", -1));
		spent->sp_warn = scale_age (getdef_num ("PASS_WARN_AGE", -1));
		spent->sp_inact = scale_age (def_inactive);
		spent->sp_expire = scale_age (user_expire);
	} else {
		spent->sp_min = scale_age (-1);
		spent->sp_max = scale_age (-1);
		spent->sp_warn = scale_age (-1);
		spent->sp_inact = scale_age (-1);
		spent->sp_expire = scale_age (-1);
	}
	spent->sp_flag = SHADOW_SP_FLAG_UNSET;
}

/*
 * grp_update - add user to secondary group set
 *
 *	grp_update() takes the secondary group set given in user_groups
 *	and adds the user to each group given by that set.
 *
 *	The group files are opened and locked in open_files().
 *
 *	close_files() should be called afterwards to commit the changes
 *	and unlocking the group files.
 */
static void grp_update (void)
{
	const struct group *grp;
	struct group *ngrp;

#ifdef	SHADOWGRP
	const struct sgrp *sgrp;
	struct sgrp *nsgrp;
#endif

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */
	for (gr_rewind (), grp = gr_next (); NULL != grp; grp = gr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		if (!is_on_list (user_groups, grp->gr_name)) {
			continue;
		}

		/*
		 * Make a copy - gr_update() will free() everything
		 * from the old entry, and we need it later.
		 */
		ngrp = __gr_dup (grp);
		if (NULL == ngrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failed to prepare the new %s entry '%s'", gr_dbname (), user_name));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding user to group",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_GRP_UPDATE);	/* XXX */
		}

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */
		ngrp->gr_mem = add_list (ngrp->gr_mem, user_name);
		if (gr_update (ngrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, gr_dbname (), ngrp->gr_name);
			SYSLOG ((LOG_ERR, "failed to prepare the new %s entry '%s'", gr_dbname (), user_name));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding user to group",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "adding user to group",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_SUCCESS);
#endif
		SYSLOG ((LOG_INFO,
		         "add '%s' to group '%s'",
		         user_name, ngrp->gr_name));
	}

#ifdef	SHADOWGRP
	if (!is_shadow_grp)
		return;

	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of. The administrative list isn't
	 * modified.
	 */
	for (sgr_rewind (), sgrp = sgr_next (); NULL != sgrp; sgrp = sgr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		if (gr_locate (sgrp->sg_name) == NULL) {
			continue;
		}

		if (!is_on_list (user_groups, sgrp->sg_name)) {
			continue;
		}

		/*
		 * Make a copy - sgr_update() will free() everything
		 * from the old entry, and we need it later.
		 */
		nsgrp = __sgr_dup (sgrp);
		if (NULL == nsgrp) {
			fprintf (stderr,
			         _("%s: Out of memory. Cannot update %s.\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to prepare the new %s entry '%s'", sgr_dbname (), user_name));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding user to shadow group",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_GRP_UPDATE);	/* XXX */
		}

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */
		nsgrp->sg_mem = add_list (nsgrp->sg_mem, user_name);
		if (sgr_update (nsgrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, sgr_dbname (), nsgrp->sg_name);
			SYSLOG ((LOG_ERR, "failed to prepare the new %s entry '%s'", sgr_dbname (), user_name));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding user to shadow group",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "adding user to shadow group",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_SUCCESS);
#endif
		SYSLOG ((LOG_INFO,
		         "add '%s' to shadow group '%s'",
		         user_name, nsgrp->sg_name));
	}
#endif				/* SHADOWGRP */
}

/*
 * process_flags - perform command line argument setting
 *
 *	process_flags() interprets the command line arguments and sets
 *	the values that the user will be created with accordingly. The
 *	values are checked for sanity.
 */
static void process_flags (int argc, char **argv)
{
	const struct group *grp;
	bool anyflag = false;
	char *cp;

	{
		/*
		 * Parse the command line options.
		 */
		int c;
		static struct option long_options[] = {
			{"base-dir", required_argument, NULL, 'b'},
			{"comment", required_argument, NULL, 'c'},
			{"home-dir", required_argument, NULL, 'd'},
			{"defaults", no_argument, NULL, 'D'},
			{"expiredate", required_argument, NULL, 'e'},
			{"inactive", required_argument, NULL, 'f'},
			{"gid", required_argument, NULL, 'g'},
			{"groups", required_argument, NULL, 'G'},
			{"help", no_argument, NULL, 'h'},
			{"skel", required_argument, NULL, 'k'},
			{"key", required_argument, NULL, 'K'},
			{"create-home", no_argument, NULL, 'm'},
			{"no-create-home", no_argument, NULL, 'M'},
			{"no-log-init", no_argument, NULL, 'l'},
			{"no-user-group", no_argument, NULL, 'N'},
			{"non-unique", no_argument, NULL, 'o'},
			{"password", required_argument, NULL, 'p'},
			{"system", no_argument, NULL, 'r'},
			{"shell", required_argument, NULL, 's'},
#ifdef WITH_SELINUX
			{"selinux-user", required_argument, NULL, 'Z'},
#endif
			{"uid", required_argument, NULL, 'u'},
			{"user-group", no_argument, NULL, 'U'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c = getopt_long (argc, argv,
#ifdef WITH_SELINUX
		                         "b:c:d:De:f:g:G:hk:K:lmMNop:rs:u:UZ:",
#else
		                         "b:c:d:De:f:g:G:hk:K:lmMNop:rs:u:U",
#endif
		                         long_options, NULL)) != -1) {
			switch (c) {
			case 'b':
				if (   ( !VALID (optarg) )
				    || ( optarg[0] != '/' )) {
					fprintf (stderr,
					         _("%s: invalid base directory '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				def_home = optarg;
				bflg = true;
				break;
			case 'c':
				if (!VALID (optarg)) {
					fprintf (stderr,
					         _("%s: invalid comment '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_comment = optarg;
				cflg = true;
				break;
			case 'd':
				if (   ( !VALID (optarg) )
				    || ( optarg[0] != '/' )) {
					fprintf (stderr,
					         _("%s: invalid home directory '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_home = optarg;
				dflg = true;
				break;
			case 'D':
				if (anyflag) {
					usage (E_USAGE);
				}
				Dflg = true;
				break;
			case 'e':
				if ('\0' != *optarg) {
					user_expire = strtoday (optarg);
					if (user_expire < -1) {
						fprintf (stderr,
						         _("%s: invalid date '%s'\n"),
						         Prog, optarg);
						exit (E_BAD_ARG);
					}
				} else {
					user_expire = -1;
				}

				/*
				 * -e "" is allowed - it's a no-op without /etc/shadow
				 */
				if (('\0' != *optarg) && !is_shadow_pwd) {
					fprintf (stderr,
					         _("%s: shadow passwords required for -e\n"),
					         Prog);
					exit (E_USAGE);
				}
				if (Dflg) {
					def_expire = optarg;
				}
				eflg = true;
				break;
			case 'f':
				if (   (getlong (optarg, &def_inactive) == 0)
				    || (def_inactive < -1)) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					usage (E_USAGE);
				}
				/*
				 * -f -1 is allowed
				 * it's a no-op without /etc/shadow
				 */
				if ((-1 != def_inactive) && !is_shadow_pwd) {
					fprintf (stderr,
					         _("%s: shadow passwords required for -f\n"),
					         Prog);
					exit (E_USAGE);
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
				if (Dflg) {
					def_group = grp->gr_gid;
					def_gname = optarg;
				} else {
					user_gid = grp->gr_gid;
				}
				gflg = true;
				break;
			case 'G':
				if (get_groups (optarg) != 0) {
					exit (E_NOTFOUND);
				}
				if (NULL != user_groups[0]) {
					do_grp_update = true;
				}
				Gflg = true;
				break;
			case 'h':
				usage (E_SUCCESS);
				break;
			case 'k':
				def_template = optarg;
				kflg = true;
				break;
			case 'K':
				/*
				 * override login.defs defaults (-K name=value)
				 * example: -K UID_MIN=100 -K UID_MAX=499
				 * note: -K UID_MIN=10,UID_MAX=499 doesn't work yet
				 */
				cp = strchr (optarg, '=');
				if (NULL == cp) {
					fprintf (stderr,
					         _("%s: -K requires KEY=VALUE\n"),
					         Prog);
					exit (E_BAD_ARG);
				}
				/* terminate name, point to value */
				*cp = '\0';
				cp++;
				if (putdef_str (optarg, cp) < 0) {
					exit (E_BAD_ARG);
				}
				break;
			case 'l':
				lflg = true;
				break;
			case 'm':
				mflg = true;
				break;
			case 'M':
				Mflg = true;
				break;
			case 'N':
				Nflg = true;
				break;
			case 'o':
				oflg = true;
				break;
			case 'p':	/* set encrypted password */
				if (!VALID (optarg)) {
					fprintf (stderr,
					         _("%s: invalid field '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_pass = optarg;
				break;
			case 'r':
				rflg = true;
				break;
			case 's':
				if (   ( !VALID (optarg) )
				    || (   ('\0' != optarg[0])
				        && ('/'  != optarg[0])
				        && ('*'  != optarg[0]) )) {
					fprintf (stderr,
					         _("%s: invalid shell '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_shell = optarg;
				def_shell = optarg;
				sflg = true;
				break;
			case 'u':
				if (   (get_uid (optarg, &user_id) == 0)
				    || (user_id == (gid_t)-1)) {
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

	if (!gflg && !Nflg && !Uflg) {
		/* Get the settings from login.defs */
		Uflg = getdef_bool ("USERGROUPS_ENAB");
	}

	/*
	 * Certain options are only valid in combination with others.
	 * Check it here so that they can be specified in any order.
	 */
	if (oflg && !uflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-o", "-u");
		usage (E_USAGE);
	}
	if (kflg && !mflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-k", "-m");
		usage (E_USAGE);
	}
	if (Uflg && gflg) {
		fprintf (stderr,
		         _("%s: options %s and %s conflict\n"),
		         Prog, "-U", "-g");
		usage (E_USAGE);
	}
	if (Uflg && Nflg) {
		fprintf (stderr,
		         _("%s: options %s and %s conflict\n"),
		         Prog, "-U", "-N");
		usage (E_USAGE);
	}
	if (mflg && Mflg) {
		fprintf (stderr,
		         _("%s: options %s and %s conflict\n"),
		         Prog, "-m", "-M");
		usage (E_USAGE);
	}

	/*
	 * Either -D or username is required. Defaults can be set with -D
	 * for the -b, -e, -f, -g, -s options only.
	 */
	if (Dflg) {
		if (optind != argc) {
			usage (E_USAGE);
		}

		if (uflg || oflg || Gflg || dflg || cflg || mflg) {
			usage (E_USAGE);
		}
	} else {
		if (optind != argc - 1) {
			usage (E_USAGE);
		}

		user_name = argv[optind];
		if (!is_valid_user_name (user_name)) {
			fprintf (stderr,
			         _("%s: invalid user name '%s'\n"),
			         Prog, user_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding user",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			exit (E_BAD_ARG);
		}
		if (!dflg) {
			char *uh;
			size_t len = strlen (def_home) + strlen (user_name) + 2;
			int wlen;

			uh = xmalloc (len);
			wlen = snprintf (uh, len, "%s/%s", def_home, user_name);
			assert (wlen == (int) len -1);

			user_home = uh;
		}
	}

	if (!eflg) {
		user_expire = strtoday (def_expire);
	}

	if (!gflg) {
		user_gid = def_group;
	}

	if (!sflg) {
		user_shell = def_shell;
	}

	create_mail_spool = def_create_mail_spool;

	if (!rflg) {
		/* for system accounts defaults are ignored and we
		 * do not create a home dir */
		if (getdef_bool ("CREATE_HOME")) {
			mflg = true;
		}
	}

	if (Mflg) {
		/* absolutely sure that we do not create home dirs */
		mflg = false;
	}
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
	if (is_shadow_pwd && (spw_close () == 0)) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"), Prog, spw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", spw_dbname ()));
		fail_exit (E_PW_UPDATE);
	}
	if (do_grp_update) {
		if (gr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"), Prog, gr_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", gr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
#ifdef	SHADOWGRP
		if (is_shadow_grp && (sgr_close () == 0)) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", sgr_dbname ()));
			fail_exit (E_GRP_UPDATE);
		}
#endif
	}
	if (is_shadow_pwd) {
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking shadow file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
		spw_locked = false;
	}
	if (pw_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "unlocking passwd file",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		/* continue */
	}
	pw_locked = false;
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "unlocking group file",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		/* continue */
	}
	gr_locked = false;
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "unlocking gshadow file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
		sgr_locked = false;
	}
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
		exit (E_PW_UPDATE);
	}
	pw_locked = true;
	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, pw_dbname ());
		fail_exit (E_PW_UPDATE);
	}

	/* shadow file will be opened by open_shadow(); */

	/*
	 * Lock and open the group file.
	 */
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
		fail_exit (E_GRP_UPDATE);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
		fail_exit (E_GRP_UPDATE);
	}
#ifdef  SHADOWGRP
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
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif
}

static void open_shadow (void)
{
	if (!is_shadow_pwd) {
		return;
	}
	if (spw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
	spw_locked = true;
	if (spw_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE);
	}
}

static char *empty_list = NULL;

/*
 * new_grent - initialize the values in a group file entry
 *
 *      new_grent() takes all of the values that have been entered and fills
 *      in a (struct group) with them.
 */

static void new_grent (struct group *grent)
{
	memzero (grent, sizeof *grent);
	grent->gr_name = (char *) user_name;
	grent->gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
	grent->gr_gid = user_gid;
	grent->gr_mem = &empty_list;
}

#ifdef  SHADOWGRP
/*
 * new_sgent - initialize the values in a shadow group file entry
 *
 *      new_sgent() takes all of the values that have been entered and fills
 *      in a (struct sgrp) with them.
 */

static void new_sgent (struct sgrp *sgent)
{
	memzero (sgent, sizeof *sgent);
	sgent->sg_name = (char *) user_name;
	sgent->sg_passwd = "!";	/* XXX warning: const */
	sgent->sg_adm = &empty_list;
	sgent->sg_mem = &empty_list;
}
#endif				/* SHADOWGRP */


/*
 * grp_add - add new group file entries
 *
 *      grp_add() writes the new records to the group files.
 */

static void grp_add (void)
{
	struct group grp;

#ifdef  SHADOWGRP
	struct sgrp sgrp;
#endif				/* SHADOWGRP */

	/*
	 * Create the initial entries for this new group.
	 */
	new_grent (&grp);
#ifdef  SHADOWGRP
	new_sgent (&sgrp);
#endif				/* SHADOWGRP */

	/*
	 * Write out the new group file entry.
	 */
	if (gr_update (&grp) == 0) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, gr_dbname (), grp.gr_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_GROUP, Prog,
		              "adding group",
		              grp.gr_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_GRP_UPDATE);
	}
#ifdef  SHADOWGRP
	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_update (&sgrp) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, sgr_dbname (), sgrp.sg_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_GROUP, Prog,
		              "adding group",
		              grp.gr_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u", user_name, user_gid));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP, Prog,
	              "adding group",
	              grp.gr_name, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
	do_grp_update = true;
}

static void faillog_reset (uid_t uid)
{
	struct faillog fl;
	int fd;
	off_t offset_uid = (off_t) (sizeof fl) * uid;

	if (access (FAILLOG_FILE, F_OK) != 0) {
		return;
	}

	memzero (&fl, sizeof (fl));

	fd = open (FAILLOG_FILE, O_RDWR);
	if (   (-1 == fd)
	    || (lseek (fd, offset_uid, SEEK_SET) != offset_uid)
	    || (write (fd, &fl, sizeof (fl)) != (ssize_t) sizeof (fl))
	    || (fsync (fd) != 0)
	    || (close (fd) != 0)) {
		fprintf (stderr,
		         _("%s: failed to reset the faillog entry of UID %lu: %s\n"),
		         Prog, (unsigned long) uid, strerror (errno));
		SYSLOG ((LOG_WARN, "failed to reset the faillog entry of UID %lu", (unsigned long) uid));
		/* continue */
	}
}

static void lastlog_reset (uid_t uid)
{
	struct lastlog ll;
	int fd;
	off_t offset_uid = (off_t) (sizeof ll) * uid;

	if (access (LASTLOG_FILE, F_OK) != 0) {
		return;
	}

	memzero (&ll, sizeof (ll));

	fd = open (LASTLOG_FILE, O_RDWR);
	if (   (-1 == fd)
	    || (lseek (fd, offset_uid, SEEK_SET) != offset_uid)
	    || (write (fd, &ll, sizeof (ll)) != (ssize_t) sizeof (ll))
	    || (fsync (fd) != 0)
	    || (close (fd) != 0)) {
		fprintf (stderr,
		         _("%s: failed to reset the lastlog entry of UID %lu: %s\n"),
		         Prog, (unsigned long) uid, strerror (errno));
		SYSLOG ((LOG_WARN, "failed to reset the lastlog entry of UID %lu", (unsigned long) uid));
		/* continue */
	}
}

/*
 * usr_update - create the user entries
 *
 *	usr_update() creates the password file entries for this user
 *	and will update the group entries if required.
 */
static void usr_update (void)
{
	struct passwd pwent;
	struct spwd spent;

	/*
	 * Fill in the password structure with any new fields, making
	 * copies of strings.
	 */
	new_pwent (&pwent);
	new_spent (&spent);

	/*
	 * Create a syslog entry. We need to do this now in case anything
	 * happens so we know what we were trying to accomplish.
	 */
	SYSLOG ((LOG_INFO,
	         "new user: name=%s, UID=%u, GID=%u, home=%s, shell=%s",
	         user_name, (unsigned int) user_id,
	         (unsigned int) user_gid, user_home, user_shell));

	/*
	 * Initialize faillog and lastlog entries for this UID in case
	 * it belongs to a previously deleted user. We do it only if
	 * no user with this UID exists yet (entries for shared UIDs
	 * are left unchanged).  --marekm
	 */
	/* local, no need for xgetpwuid */
	if ((!lflg) && (getpwuid (user_id) == NULL)) {
		faillog_reset (user_id);
		lastlog_reset (user_id);
	}

	/*
	 * Put the new (struct passwd) in the table.
	 */
	if (pw_update (&pwent) == 0) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, pw_dbname (), pwent.pw_name);
		fail_exit (E_PW_UPDATE);
	}

	/*
	 * Put the new (struct spwd) in the table.
	 */
	if (is_shadow_pwd && (spw_update (&spent) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, spw_dbname (), spent.sp_namp);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "adding shadow password",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_PW_UPDATE);
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_USER, Prog,
	              "adding user",
	              user_name, (unsigned int) user_id,
	              SHADOW_AUDIT_SUCCESS);
#endif

	/*
	 * Do any group file updates for this user.
	 */
	if (do_grp_update) {
		grp_update ();
	}
}

#ifdef WITH_SELINUX
static void selinux_update_mapping (void) {
	if (is_selinux_enabled () <= 0) return;

	if ('\0' != *user_selinux) { /* must be done after passwd write() */
		const char *argv[7];
		argv[0] = "/usr/sbin/semanage";
		argv[1] = "login";
		argv[2] = "-a";
		argv[3] = "-s";
		argv[4] = user_selinux;
		argv[5] = user_name;
		argv[6] = NULL;
		if (safe_system (argv[0], argv, NULL, false) != 0) {
			fprintf (stderr,
			         _("%s: warning: the user name %s to %s SELinux user mapping failed.\n"),
			         Prog, user_name, user_selinux);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding SELinux user mapping",
			              user_name, (unsigned int) user_id, 0);
#endif
		}
	}
}
#endif
/*
 * create_home - create the user's home directory
 *
 *	create_home() creates the user's home directory if it does not
 *	already exist. It will be created mode 755 owned by the user
 *	with the user's default group.
 */
static void create_home (void)
{
	if (access (user_home, F_OK) != 0) {
#ifdef WITH_SELINUX
		if (set_selinux_file_context (user_home) != 0) {
			fail_exit (E_HOMEDIR);
		}
#endif
		/* XXX - create missing parent directories.  --marekm */
		if (mkdir (user_home, 0) != 0) {
			fprintf (stderr,
			         _("%s: cannot create directory %s\n"),
			         Prog, user_home);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding home directory",
			              user_name, (unsigned int) user_id,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_HOMEDIR);
		}
		chown (user_home, user_id, user_gid);
		chmod (user_home,
		       0777 & ~getdef_num ("UMASK", GETDEF_DEFAULT_UMASK));
		home_added = true;
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "adding home directory",
		              user_name, (unsigned int) user_id,
		              SHADOW_AUDIT_SUCCESS);
#endif
#ifdef WITH_SELINUX
		/* Reset SELinux to create files with default contexts */
		if (reset_selinux_file_context () != 0) {
			fail_exit (E_HOMEDIR);
		}
#endif
	}
}

/*
 * create_mail - create the user's mail spool
 *
 *	create_mail() creates the user's mail spool if it does not already
 *	exist. It will be created mode 660 owned by the user and group
 *	'mail'
 */
static void create_mail (void)
{
	if (strcasecmp (create_mail_spool, "yes") == 0) {
		const char *spool;
		char *file;
		int fd;
		struct group *gr;
		gid_t gid;
		mode_t mode;

		spool = getdef_str ("MAIL_DIR");
		if (NULL == spool) {
			spool = "/var/mail";
		}
		file = alloca (strlen (spool) + strlen (user_name) + 2);
		sprintf (file, "%s/%s", spool, user_name);
		fd = open (file, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0);
		if (fd < 0) {
			perror (_("Creating mailbox file"));
			return;
		}

		gr = getgrnam ("mail"); /* local, no need for xgetgrnam */
		if (NULL == gr) {
			fputs (_("Group 'mail' not found. Creating the user mailbox file with 0600 mode.\n"),
			       stderr);
			gid = user_gid;
			mode = 0600;
		} else {
			gid = gr->gr_gid;
			mode = 0660;
		}

		if (   (fchown (fd, user_id, gid) != 0)
		    || (fchmod (fd, mode) != 0)) {
			perror (_("Setting mailbox file permissions"));
		}

		fsync (fd);
		close (fd);
	}
}

/*
 * main - useradd command
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

	OPENLOG ("useradd");

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	user_groups = (char **) xmalloc ((1 + sys_ngroups) * sizeof (char *));
	/*
	 * Initialize the list to be empty
	 */
	user_groups[0] = (char *) 0;


	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	get_defaults ();

	process_flags (argc, argv);

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			fail_exit (1);
		}

		retval = pam_start ("useradd", pampw->pw_name, &conv, &pamh);
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
		fail_exit (1);
	}
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */

	/*
	 * See if we are messing with the defaults file, or creating
	 * a new user.
	 */
	if (Dflg) {
		if (gflg || bflg || fflg || eflg || sflg) {
			exit ((set_defaults () != 0) ? 1 : 0);
		}

		show_defaults ();
		exit (E_SUCCESS);
	}

	/*
	 * Start with a quick check to see if the user exists.
	 */
	if (getpwnam (user_name) != NULL) { /* local, no need for xgetpwnam */
		fprintf (stderr, _("%s: user '%s' already exists\n"), Prog, user_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER, Prog,
		              "adding user",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_NAME_IN_USE);
	}

	/*
	 * Don't blindly overwrite a group when a user is added...
	 * If you already have a group username, and want to add the user
	 * to that group, use useradd -g username username.
	 * --bero
	 */
	if (Uflg) {
		/* local, no need for xgetgrnam */
		if (getgrnam (user_name) != NULL) {
			fprintf (stderr,
			         _("%s: group %s exists - if you want to add this user to that group, use -g.\n"),
			         Prog, user_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER, Prog,
			              "adding group",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			fail_exit (E_NAME_IN_USE);
		}
	}

	/*
	 * Do the hard stuff:
	 * - open the files,
	 * - create the user entries,
	 * - create the home directory,
	 * - create user mail spool,
	 * - flush nscd caches for passwd and group services,
	 * - then close and update the files.
	 */
	open_files ();

	if (!oflg) {
		/* first, seek for a valid uid to use for this user.
		 * We do this because later we can use the uid we found as
		 * gid too ... --gafton */
		if (!uflg) {
			if (find_new_uid (rflg, &user_id, NULL) < 0) {
				fprintf (stderr, _("%s: can't create user\n"), Prog);
				fail_exit (E_UID_IN_USE);
			}
		} else {
			if (getpwuid (user_id) != NULL) {
				fprintf (stderr,
				         _("%s: UID %lu is not unique\n"),
				         Prog, (unsigned long) user_id);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_ADD_USER, Prog,
				              "adding user",
				              user_name, (unsigned int) user_id,
				              SHADOW_AUDIT_FAILURE);
#endif
				fail_exit (E_UID_IN_USE);
			}
		}
	}

#ifdef WITH_TCB
	if (getdef_bool ("USE_TCB")) {
		if (shadowtcb_create (user_name, user_id) == SHADOWTCB_FAILURE) {
			fprintf (stderr,
			         _("%s: Failed to create tcb directory for %s\n"),
			         Prog, user_name);
			fail_exit (E_UID_IN_USE);
		}
	}
#endif
	open_shadow ();

	/* do we have to add a group for that user? This is why we need to
	 * open the group files in the open_files() function  --gafton */
	if (Uflg) {
		if (find_new_gid (rflg, &user_gid, &user_id) < 0) {
			fprintf (stderr,
			         _("%s: can't create group\n"),
			         Prog);
			fail_exit (4);
		}
		grp_add ();
	}

	usr_update ();

	if (mflg) {
		create_home ();
		if (home_added) {
			copy_tree (def_template, user_home, false, false,
			           (uid_t)-1, user_id, (gid_t)-1, user_gid);
		} else {
			fprintf (stderr,
			         _("%s: warning: the home directory already exists.\n"
			           "Not copying any file from skel directory into it.\n"),
			         Prog);
		}

	}

	/* Do not create mail directory for system accounts */
	if (!rflg) {
		create_mail ();
	}

	close_files ();

#ifdef WITH_SELINUX
	selinux_update_mapping ();
#endif

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	return E_SUCCESS;
}

