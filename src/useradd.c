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

#include <assert.h>
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

static long user_expire = -1;
static int is_shadow_pwd;

#ifdef SHADOWGRP
static int is_shadow_grp;
static int gshadow_locked = 0;
#endif
static int passwd_locked = 0;
static int group_locked = 0;
static int shadow_locked = 0;
static char **user_groups;	/* NULL-terminated list */
static long sys_ngroups;
static int do_grp_update = 0;	/* group files need to be updated */

static char *Prog;

static int
    bflg = 0,			/* new default root of home directory */
    cflg = 0,			/* comment (GECOS) field for new account */
    dflg = 0,			/* home directory for new account */
    Dflg = 0,			/* set/show new user default values */
    eflg = 0,			/* days since 1970-01-01 when account is locked */
    fflg = 0,			/* days until account with expired password is locked */
    gflg = 0,			/* primary group ID for new account */
    Gflg = 0,			/* secondary group set for new account */
    kflg = 0,			/* specify a directory to fill new user directory */
    lflg = 0,			/* do not add user to lastlog database file */
    mflg = 0,			/* create user's home directory if it doesn't exist */
    Nflg = 0,			/* do not create a group having the same name as the user, but add the user to def_group (or the group specified with -g) */
    oflg = 0,			/* permit non-unique user ID to be specified with -u */
    rflg = 0,			/* create a system account */
    sflg = 0,			/* shell program for new account */
    uflg = 0,			/* specify user ID for new account */
    Uflg = 0;			/* create a group having the same name as the user */

static int home_added;

/*
 * exit status values
 */
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
#define HOME			"HOME="
#define SHELL			"SHELL="
#define INACT			"INACTIVE="
#define EXPIRE			"EXPIRE="
#define SKEL			"SKEL="
#define CREATE_MAIL_SPOOL	"CREATE_MAIL_SPOOL="

/* local function prototypes */
static void fail_exit (int);
static struct group *getgr_nam_gid (const char *);
static long get_number (const char *);
static uid_t get_uid (const char *);
static void get_defaults (void);
static void show_defaults (void);
static int set_defaults (void);
static int get_groups (char *);
static void usage (void);
static void new_pwent (struct passwd *);

static long scale_age (long);
static void new_spent (struct spwd *);
static void grp_update (void);

static void process_flags (int argc, char **argv);
static void close_files (void);
static void open_files (void);
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
	if (home_added)
		rmdir (user_home);

	if (shadow_locked) {
		spw_unlock ();
	}
	if (passwd_locked) {
		pw_unlock ();
	}
	if (group_locked) {
		gr_unlock ();
	}
#ifdef	SHADOWGRP
	if (gshadow_locked) {
		sgr_unlock ();
	}
#endif

#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding user", user_name, -1,
		      0);
#endif
	SYSLOG ((LOG_INFO, "failed adding user `%s', data deleted", user_name));
	exit (code);
}

static struct group *getgr_nam_gid (const char *grname)
{
	long gid;
	char *errptr;

	gid = strtol (grname, &errptr, 10);
	if (*grname != '\0' && *errptr == '\0' && errno != ERANGE && gid >= 0)
		return xgetgrgid (gid);
	return xgetgrnam (grname);
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

static uid_t get_uid (const char *uidstr)
{
	long val;
	char *errptr;

	val = strtol (uidstr, &errptr, 10);
	if (*errptr || errno == ERANGE || val < 0) {
		fprintf (stderr,
			 _("%s: invalid numeric argument '%s'\n"), Prog,
			 uidstr);
		exit (E_BAD_ARG);
	}
	return val;
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
	char *cp, *ep;

	/*
	 * Open the defaults file for reading.
	 */

	if (!(fp = fopen (def_file, "r")))
		return;

	/*
	 * Read the file a line at a time. Only the lines that have relevant
	 * values are used, everything else can be ignored.
	 */
	while (fgets (buf, sizeof buf, fp)) {
		if ((cp = strrchr (buf, '\n')))
			*cp = '\0';

		if (!(cp = strchr (buf, '=')))
			continue;

		cp++;

		/*
		 * Primary GROUP identifier
		 */
		if (MATCH (buf, DGROUP)) {
			unsigned int val = (unsigned int) strtoul (cp, &ep, 10);
			const struct group *grp;

			if (*cp != '\0' && *ep == '\0') { /* valid number */
				def_group = val;
				/* local, no need for xgetgrgid */
				if ((grp = getgrgid (def_group))) {
					def_gname = xstrdup (grp->gr_name);
				} else {
					fprintf (stderr,
						 _("%s: unknown GID %s\n"),
						 Prog, cp);
				}
			/* local, no need for xgetgrnam */
			} else if ((grp = getgrnam (cp))) {
				def_group = grp->gr_gid;
				def_gname = xstrdup (cp);
			} else {
				fprintf (stderr,
					 _("%s: unknown group %s\n"), Prog, cp);
			}
		}

		/*
		 * Default HOME filesystem
		 */
		else if (MATCH (buf, HOME)) {
			def_home = xstrdup (cp);
		}

		/*
		 * Default Login Shell command
		 */
		else if (MATCH (buf, SHELL)) {
			def_shell = xstrdup (cp);
		}

		/*
		 * Default Password Inactive value
		 */
		else if (MATCH (buf, INACT)) {
			long val = strtol (cp, &ep, 10);

			if (*cp || errno == ERANGE)
				def_inactive = val;
			else
				def_inactive = -1;
		}

		/*
		 * Default account expiration date
		 */
		else if (MATCH (buf, EXPIRE)) {
			def_expire = xstrdup (cp);
		}

		/*
		 * Default Skeleton information
		 */
		else if (MATCH (buf, SKEL)) {
			if (*cp == '\0')
				cp = SKEL_DIR;	/* XXX warning: const */

			def_template = xstrdup (cp);
		}

		/*
		 * Create by default user mail spool or not ?
		 */
		else if (MATCH (buf, CREATE_MAIL_SPOOL)) {
			if (*cp == '\0')
				cp = CREATE_MAIL_SPOOL;	/* XXX warning: const */

			def_create_mail_spool = xstrdup (cp);
		}
	}
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
	int out_group = 0;
	int out_home = 0;
	int out_inactive = 0;
	int out_expire = 0;
	int out_shell = 0;
	int out_skel = 0;
	int out_create_mail_spool = 0;

	/*
	 * Create a temporary file to copy the new output to.
	 */
	if ((ofd = mkstemp (new_file)) == -1) {
		fprintf (stderr,
			 _("%s: cannot create new defaults file\n"), Prog);
		return -1;
	}

	if (!(ofp = fdopen (ofd, "w"))) {
		fprintf (stderr, _("%s: cannot open new defaults file\n"),
			 Prog);
		return -1;
	}

	/*
	 * Open the existing defaults file and copy the lines to the
	 * temporary file, using any new values. Each line is checked
	 * to insure that it is not output more than once.
	 */
	if (!(ifp = fopen (def_file, "r"))) {
		fprintf (ofp, "# useradd defaults file\n");
		goto skip;
	}

	while (fgets (buf, sizeof buf, ifp)) {
		if ((cp = strrchr (buf, '\n')))
			*cp = '\0';

		if (!out_group && MATCH (buf, DGROUP)) {
			fprintf (ofp, DGROUP "%u\n", (unsigned int) def_group);
			out_group++;
		} else if (!out_home && MATCH (buf, HOME)) {
			fprintf (ofp, HOME "%s\n", def_home);
			out_home++;
		} else if (!out_inactive && MATCH (buf, INACT)) {
			fprintf (ofp, INACT "%ld\n", def_inactive);
			out_inactive++;
		} else if (!out_expire && MATCH (buf, EXPIRE)) {
			fprintf (ofp, EXPIRE "%s\n", def_expire);
			out_expire++;
		} else if (!out_shell && MATCH (buf, SHELL)) {
			fprintf (ofp, SHELL "%s\n", def_shell);
			out_shell++;
		} else if (!out_skel && MATCH (buf, SKEL)) {
			fprintf (ofp, SKEL "%s\n", def_template);
			out_skel++;
		} else if (!out_create_mail_spool
			   && MATCH (buf, CREATE_MAIL_SPOOL)) {
			fprintf (ofp, CREATE_MAIL_SPOOL "%s\n",
				 def_create_mail_spool);
			out_create_mail_spool++;
		} else
			fprintf (ofp, "%s\n", buf);
	}
	fclose (ifp);

      skip:
	/*
	 * Check each line to insure that every line was output. This
	 * causes new values to be added to a file which did not previously
	 * have an entry for that value.
	 */
	if (!out_group)
		fprintf (ofp, DGROUP "%u\n", (unsigned int) def_group);
	if (!out_home)
		fprintf (ofp, HOME "%s\n", def_home);
	if (!out_inactive)
		fprintf (ofp, INACT "%ld\n", def_inactive);
	if (!out_expire)
		fprintf (ofp, EXPIRE "%s\n", def_expire);
	if (!out_shell)
		fprintf (ofp, SHELL "%s\n", def_shell);
	if (!out_skel)
		fprintf (ofp, SKEL "%s\n", def_template);

	if (!out_create_mail_spool)
		fprintf (ofp, CREATE_MAIL_SPOOL "%s\n", def_create_mail_spool);

	/*
	 * Flush and close the file. Check for errors to make certain
	 * the new file is intact.
	 */
	fflush (ofp);
	if (ferror (ofp) || fclose (ofp)) {
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the current default file to its backup name.
	 */
	snprintf (buf, sizeof buf, "%s-", def_file);
	if (rename (def_file, buf) && errno != ENOENT) {
		snprintf (buf, sizeof buf, _("%s: rename: %s"), Prog, def_file);
		perror (buf);
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the new default file to its correct name.
	 */
	if (rename (new_file, def_file)) {
		snprintf (buf, sizeof buf, _("%s: rename: %s"), Prog, new_file);
		perror (buf);
		return -1;
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "changing user defaults",
		      NULL, -1, 1);
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

	if (!*list)
		return 0;

	/*
	 * So long as there is some data to be converted, strip off
	 * each name and look it up. A mix of numerical and string
	 * values for group identifiers is permitted.
	 */
	do {
		/*
		 * Strip off a single name from the list
		 */
		if ((cp = strchr (list, ',')))
			*cp++ = '\0';

		/*
		 * Names starting with digits are treated as numerical
		 * GID values, otherwise the string is looked up as is.
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
		 * If the group doesn't exist, don't dump core...
		 * Instead, try the next one.  --marekm
		 */
		if (!grp)
			continue;

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
	fputs (_("Usage: useradd [options] LOGIN\n"
	         "\n"
	         "Options:\n"
	         "  -b, --base-dir BASE_DIR       base directory for the new user account\n"
	         "                                home directory\n"
	         "  -c, --comment COMMENT         set the GECOS field for the new user account\n"
	         "  -d, --home-dir HOME_DIR       home directory for the new user account\n"
	         "  -D, --defaults                print or save modified default useradd\n"
	         "                                configuration\n"
	         "  -e, --expiredate EXPIRE_DATE  set account expiration date to EXPIRE_DATE\n"
	         "  -f, --inactive INACTIVE       set password inactive after expiration\n"
	         "                                to INACTIVE\n"
	         "  -g, --gid GROUP               force use GROUP for the new user account\n"
	         "  -G, --groups GROUPS           list of supplementary groups for the new\n"
	         "                                user account\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -k, --skel SKEL_DIR           specify an alternative skel directory\n"
	         "  -K, --key KEY=VALUE           overrides /etc/login.defs defaults\n"
	         "  -l,                           do not add the user to the lastlog and\n"
	         "                                faillog databases\n"
	         "  -m, --create-home             create home directory for the new user\n"
	         "                                account\n"
	         "  -N, --no-user-group           do not create a group with the same name as\n"
	         "                                the user\n"
	         "  -o, --non-unique              allow create user with duplicate\n"
	         "                                (non-unique) UID\n"
	         "  -p, --password PASSWORD       use encrypted password for the new user\n"
	         "                                account\n"
	         "  -r, --system                  create a system account\n"
	         "  -s, --shell SHELL             the login shell for the new user account\n"
	         "  -u, --uid UID                 force use the UID for the new user account\n"
	         "  -U, --user-group              create a group with the same name as the user\n"
	         "\n"), stderr);
	exit (E_USAGE);
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
	if (is_shadow_pwd)
		pwent->pw_passwd = (char *) SHADOW_PASSWD_STRING;
	else
		pwent->pw_passwd = (char *) user_pass;

	pwent->pw_uid = user_id;
	pwent->pw_gid = user_gid;
	pwent->pw_gecos = (char *) user_comment;
	pwent->pw_dir = (char *) user_home;
	pwent->pw_shell = (char *) user_shell;
}

static long scale_age (long x)
{
	if (x <= 0)
		return x;

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
	spent->sp_lstchg = time ((time_t *) 0) / SCALE;
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
	spent->sp_flag = -1;
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
	for (gr_rewind (), grp = gr_next (); grp; grp = gr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		if (!is_on_list (user_groups, grp->gr_name))
			continue;

		/*
		 * Make a copy - gr_update() will free() everything
		 * from the old entry, and we need it later.
		 */
		ngrp = __gr_dup (grp);
		if (!ngrp) {
			fprintf (stderr,
				 _("%s: Out of memory. Cannot update the group database.\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);	/* XXX */
		}

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */
		ngrp->gr_mem = add_list (ngrp->gr_mem, user_name);
		if (!gr_update (ngrp)) {
			fprintf (stderr,
				 _("%s: error adding new group entry\n"), Prog);
			fail_exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "adding user to group", user_name, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "add `%s' to group `%s'",
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
	for (sgr_rewind (), sgrp = sgr_next (); sgrp; sgrp = sgr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */
		if (!gr_locate (sgrp->sg_name))
			continue;

		if (!is_on_list (user_groups, sgrp->sg_name))
			continue;

		/*
		 * Make a copy - sgr_update() will free() everything
		 * from the old entry, and we need it later.
		 */
		nsgrp = __sgr_dup (sgrp);
		if (!nsgrp) {
			fprintf (stderr,
				 _("%s: Out of memory. Cannot update the shadow group database.\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);	/* XXX */
		}

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */
		nsgrp->sg_mem = add_list (nsgrp->sg_mem, user_name);
		if (!sgr_update (nsgrp)) {
			fprintf (stderr,
				 _("%s: error adding new group entry\n"), Prog);
			fail_exit (E_GRP_UPDATE);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "adding user to shadow group", user_name, -1, 1);
#endif
		SYSLOG ((LOG_INFO, "add `%s' to shadow group `%s'",
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
	int anyflag = 0;
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
			{"no-user-group", no_argument, NULL, 'N'},
			{"non-unique", no_argument, NULL, 'o'},
			{"password", required_argument, NULL, 'p'},
			{"system", no_argument, NULL, 'r'},
			{"shell", required_argument, NULL, 's'},
			{"uid", required_argument, NULL, 'u'},
			{"user-group", no_argument, NULL, 'U'},
			{NULL, 0, NULL, '\0'}
		};
		while ((c =
			getopt_long (argc, argv, "b:c:d:De:f:g:G:k:K:lmMNop:rs:u:U",
				     long_options, NULL)) != -1) {
			switch (c) {
			case 'b':
				if (!VALID (optarg)
				    || optarg[0] != '/') {
					fprintf (stderr,
						 _
						 ("%s: invalid base directory '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				def_home = optarg;
				bflg++;
				break;
			case 'c':
				if (!VALID (optarg)) {
					fprintf (stderr,
						 _
						 ("%s: invalid comment '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_comment = optarg;
				cflg++;
				break;
			case 'd':
				if (!VALID (optarg)
				    || optarg[0] != '/') {
					fprintf (stderr,
						 _
						 ("%s: invalid home directory '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_home = optarg;
				dflg++;
				break;
			case 'D':
				if (anyflag)
					usage ();
				Dflg++;
				break;
			case 'e':
				if (*optarg) {
					user_expire = strtoday (optarg);
					if (user_expire == -1) {
						fprintf (stderr,
							 _
							 ("%s: invalid date '%s'\n"),
							 Prog, optarg);
						exit (E_BAD_ARG);
					}
				} else
					user_expire = -1;

				/*
				 * -e "" is allowed - it's a no-op without /etc/shadow
				 */
				if (*optarg && !is_shadow_pwd) {
					fprintf (stderr,
						 _
						 ("%s: shadow passwords required for -e\n"),
						 Prog);
					exit (E_USAGE);
				}
				if (Dflg)
					def_expire = optarg;
				eflg++;
				break;
			case 'f':
				def_inactive = get_number (optarg);
				/*
				 * -f -1 is allowed - it's a no-op without /etc/shadow
				 */
				if (def_inactive != -1 && !is_shadow_pwd) {
					fprintf (stderr,
						 _
						 ("%s: shadow passwords required for -f\n"),
						 Prog);
					exit (E_USAGE);
				}
				fflg++;
				break;
			case 'g':
				grp = getgr_nam_gid (optarg);
				if (!grp) {
					fprintf (stderr,
						 _
						 ("%s: unknown group %s\n"),
						 Prog, optarg);
					exit (E_NOTFOUND);
				}
				if (Dflg) {
					def_group = grp->gr_gid;
					def_gname = optarg;
				} else {
					user_gid = grp->gr_gid;
				}
				gflg++;
				break;
			case 'G':
				if (get_groups (optarg))
					exit (E_NOTFOUND);
				if (user_groups[0])
					do_grp_update++;
				Gflg++;
				break;
			case 'h':
				usage ();
				break;
			case 'k':
				def_template = optarg;
				kflg++;
				break;
			case 'K':
				/*
				 * override login.defs defaults (-K name=value)
				 * example: -K UID_MIN=100 -K UID_MAX=499
				 * note: -K UID_MIN=10,UID_MAX=499 doesn't work yet
				 */
				cp = strchr (optarg, '=');
				if (!cp) {
					fprintf (stderr,
						 _
						 ("%s: -K requires KEY=VALUE\n"),
						 Prog);
					exit (E_BAD_ARG);
				}
				/* terminate name, point to value */
				*cp++ = '\0';
				if (putdef_str (optarg, cp) < 0)
					exit (E_BAD_ARG);
				break;
			case 'l':
				lflg++;
				break;
			case 'm':
				mflg++;
				break;
			case 'N':
				Nflg++;
				break;
			case 'o':
				oflg++;
				break;
			case 'p':	/* set encrypted password */
				if (!VALID (optarg)) {
					fprintf (stderr,
						 _
						 ("%s: invalid field '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_pass = optarg;
				break;
			case 'r':
				rflg++;
				break;
			case 's':
				if (!VALID (optarg)
				    || (optarg[0]
					&& (optarg[0] != '/'
					    && optarg[0] != '*'))) {
					fprintf (stderr,
						 _
						 ("%s: invalid shell '%s'\n"),
						 Prog, optarg);
					exit (E_BAD_ARG);
				}
				user_shell = optarg;
				def_shell = optarg;
				sflg++;
				break;
			case 'u':
				user_id = get_uid (optarg);
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
		         _("%s: %s flag is ONLY allowed with the %s flag\n"),
		         Prog, "-o", "-u");
		usage ();
	}
	if (kflg && !mflg) {
		fprintf (stderr,
		         _("%s: %s flag is ONLY allowed with the %s flag\n"),
		         Prog, "-k", "-m");
		usage ();
	}
	if (Uflg && gflg) {
		fprintf (stderr,
		         _("%s: options %s and %s conflict\n"),
		         Prog, "-U", "-g");
		usage ();
	}
	if (Uflg && Nflg) {
		fprintf (stderr,
		         _("%s: options %s and %s conflict\n"),
		         Prog, "-U", "-N");
		usage ();
	}

	/*
	 * Either -D or username is required. Defaults can be set with -D
	 * for the -b, -e, -f, -g, -s options only.
	 */
	if (Dflg) {
		if (optind != argc)
			usage ();

		if (uflg || oflg || Gflg || dflg || cflg || mflg)
			usage ();
	} else {
		if (optind != argc - 1)
			usage ();

		user_name = argv[optind];
		if (!is_valid_user_name (user_name)) {
			fprintf (stderr,
				 _
				 ("%s: invalid user name '%s'\n"),
				 Prog, user_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding user",
				      user_name, -1, 0);
#endif
			exit (E_BAD_ARG);
		}
		if (!dflg) {
			char *uh;

			uh = xmalloc (strlen (def_home) +
				      strlen (user_name) + 2);
			sprintf (uh, "%s/%s", def_home, user_name);
			user_home = uh;
		}
	}

	if (!eflg)
		user_expire = strtoday (def_expire);

	if (!gflg)
		user_gid = def_group;

	if (!sflg)
		user_shell = def_shell;

	/* TODO: add handle change default spool mail creation by 
	   -K CREATE_MAIL_SPOOL={yes,no}. It need rewrite internal API for handle
	   shadow tools configuration */
	create_mail_spool = def_create_mail_spool;
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new user. This causes any modified entries to be written out.
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
	if (do_grp_update) {
		if (!gr_close ()) {
			fprintf (stderr,
				 _("%s: cannot rewrite group file\n"), Prog);
			fail_exit (E_GRP_UPDATE);
		}
#ifdef	SHADOWGRP
		if (is_shadow_grp && !sgr_close ()) {
			fprintf (stderr,
				 _
				 ("%s: cannot rewrite shadow group file\n"),
				 Prog);
			fail_exit (E_GRP_UPDATE);
		}
#endif
	}
	if (is_shadow_pwd) {
		spw_unlock ();
		shadow_locked--;
	}
	pw_unlock ();
	passwd_locked--;
	gr_unlock ();
	group_locked--;
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		sgr_unlock ();
		gshadow_locked--;
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
	if (!pw_lock ()) {
		fprintf (stderr, _("%s: unable to lock password file\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "locking password file", user_name, user_id, 0);
#endif
		exit (E_PW_UPDATE);
	}
	passwd_locked++;
	if (!pw_open (O_RDWR)) {
		fprintf (stderr, _("%s: unable to open password file\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "opening password file", user_name, user_id, 0);
#endif
		fail_exit (E_PW_UPDATE);
	}
	if (is_shadow_pwd) {
		if (!spw_lock ()) {
			fprintf (stderr,
			         _("%s: cannot lock shadow password file\n"),
			         Prog);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "locking shadow password file", user_name,
			              user_id, 0);
#endif
			fail_exit (E_PW_UPDATE);
		}
		shadow_locked++;
		if (!spw_open (O_RDWR)) {
			fprintf (stderr,
			         _("%s: cannot open shadow password file\n"),
			         Prog);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "opening shadow password file", user_name,
			              user_id, 0);
#endif
			fail_exit (E_PW_UPDATE);
		}
	}

	/*
	 * Lock and open the group file.
	 */
	if (!gr_lock ()) {
		fprintf (stderr, _("%s: error locking group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
	group_locked++;
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: error opening group file\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#ifdef  SHADOWGRP
	if (is_shadow_grp) {
		if (!sgr_lock ()) {
			fprintf (stderr,
			         _("%s: error locking shadow group file\n"),
			         Prog);
			fail_exit (E_GRP_UPDATE);
		}
		gshadow_locked++;
		if (!sgr_open (O_RDWR)) {
			fprintf (stderr,
			         _("%s: error opening shadow group file\n"),
			         Prog);
			fail_exit (E_GRP_UPDATE);
		}
	}
#endif
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
	if (!gr_update (&grp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#ifdef  SHADOWGRP
	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && !sgr_update (&sgrp)) {
		fprintf (stderr, _("%s: error adding new group entry\n"), Prog);
		fail_exit (E_GRP_UPDATE);
	}
#endif				/* SHADOWGRP */
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u", user_name, user_gid));
	do_grp_update++;
}

static void faillog_reset (uid_t uid)
{
	struct faillog fl;
	int fd;

	fd = open (FAILLOG_FILE, O_RDWR);
	if (fd >= 0) {
		memzero (&fl, sizeof (fl));
		lseek (fd, (off_t) sizeof (fl) * uid, SEEK_SET);
		write (fd, &fl, sizeof (fl));
		close (fd);
	}
}

static void lastlog_reset (uid_t uid)
{
	struct lastlog ll;
	int fd;

	fd = open (LASTLOG_FILE, O_RDWR);
	if (fd >= 0) {
		memzero (&ll, sizeof (ll));
		lseek (fd, (off_t) sizeof (ll) * uid, SEEK_SET);
		write (fd, &ll, sizeof (ll));
		close (fd);
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
	if (!pw_update (&pwent)) {
		fprintf (stderr,
			 _("%s: error adding new password entry\n"), Prog);
		fail_exit (E_PW_UPDATE);
	}

	/*
	 * Put the new (struct spwd) in the table.
	 */
	if (is_shadow_pwd && !spw_update (&spent)) {
		fprintf (stderr,
			 _
			 ("%s: error adding new shadow password entry\n"),
			 Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "adding shadow password", user_name, user_id, 0);
#endif
		fail_exit (E_PW_UPDATE);
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding user", user_name,
		      user_id, 1);
#endif

	/*
	 * Do any group file updates for this user.
	 */
	if (do_grp_update)
		grp_update ();
}

/*
 * create_home - create the user's home directory
 *
 *	create_home() creates the user's home directory if it does not
 *	already exist. It will be created mode 755 owned by the user
 *	with the user's default group.
 */
static void create_home (void)
{
	if (access (user_home, F_OK)) {
		/* XXX - create missing parent directories.  --marekm */
		if (mkdir (user_home, 0)) {
			fprintf (stderr,
				 _
				 ("%s: cannot create directory %s\n"),
				 Prog, user_home);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "adding home directory", user_name,
				      user_id, 0);
#endif
			fail_exit (E_HOMEDIR);
		}
		chown (user_home, user_id, user_gid);
		chmod (user_home,
		       0777 & ~getdef_num ("UMASK", GETDEF_DEFAULT_UMASK));
		home_added++;
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			      "adding home directory", user_name, user_id, 1);
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
	char *spool, *file;
	int fd;
	struct group *gr;
	gid_t gid;
	mode_t mode;

	if (strcasecmp (create_mail_spool, "yes") == 0) {
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
		if (!gr) {
			fputs (_("Group 'mail' not found. Creating the user mailbox file with 0600 mode.\n"),
			       stderr);
			gid = user_gid;
			mode = 0600;
		} else {
			gid = gr->gr_gid;
			mode = 0660;
		}

		if (fchown (fd, user_id, gid) || fchmod (fd, mode))
			perror (_("Setting mailbox file permissions"));

		close (fd);
	}
}

/*
 * main - useradd command
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

	OPENLOG ("useradd");

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	user_groups = malloc ((1 + sys_ngroups) * sizeof (char *));
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

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL) {
			retval = PAM_USER_UNKNOWN;
		}

		if (retval == PAM_SUCCESS) {
			retval = pam_start ("useradd", pampw->pw_name,
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
		fail_exit (1);
	}
#endif				/* USE_PAM */

	/*
	 * See if we are messing with the defaults file, or creating
	 * a new user.
	 */
	if (Dflg) {
		if (gflg || bflg || fflg || eflg || sflg)
			exit (set_defaults ()? 1 : 0);

		show_defaults ();
		exit (E_SUCCESS);
	}

	/*
	 * Start with a quick check to see if the user exists.
	 */
	if (getpwnam (user_name)) { /* local, no need for xgetpwnam */
		fprintf (stderr, _("%s: user %s exists\n"), Prog, user_name);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding user",
			      user_name, -1, 0);
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
		if (getgrnam (user_name)) { /* local, no need for xgetgrnam */
			fprintf (stderr,
				 _
				 ("%s: group %s exists - if you want to add this user to that group, use -g.\n"),
				 Prog, user_name);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
				      "adding group", user_name, -1, 0);
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
				fprintf (stderr, _("%s: UID %u is not unique\n"), Prog, (unsigned int) user_id);
#ifdef WITH_AUDIT
				audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "adding user", user_name, user_id, 0);
#endif
				fail_exit (E_UID_IN_USE);
			}
		}
	}

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
		if (home_added)
			copy_tree (def_template, user_home, user_id, user_gid);
		else
			fprintf (stderr,
				 _
				 ("%s: warning: the home directory already exists.\n"
				  "Not copying any file from skel directory into it.\n"),
				 Prog);

	} else if (getdef_str ("CREATE_HOME")) {
		/*
		 * RedHat added the CREATE_HOME option in login.defs in their
		 * version of shadow-utils (which makes -m the default, with
		 * new -M option to turn it off). Unfortunately, this
		 * changes the way useradd works (it can be run by scripts
		 * expecting some standard behaviour), compared to other
		 * Unices and other Linux distributions, and also adds a lot
		 * of confusion :-(.
		 * So we now recognize CREATE_HOME and give a warning here
		 * (better than "configuration error ... notify administrator"
		 * errors in every program that reads /etc/login.defs). -MM
		 */
		fprintf (stderr,
			 _
			 ("%s: warning: CREATE_HOME not supported, please use -m instead.\n"),
			 Prog);
	}

	create_mail ();

	close_files ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

#ifdef USE_PAM
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	return E_SUCCESS;
}
