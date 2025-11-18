/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2000 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2012, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#ident "$Id$"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#ifdef ENABLE_LASTLOG
#include <lastlog.h>
#endif /* ENABLE_LASTLOG */
#include <libgen.h>
#include <pwd.h>
#include <signal.h>
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "alloc/malloc.h"
#include "atoi/a2i/a2s.h"
#include "atoi/getnum.h"
#include "chkname.h"
#include "defines.h"
#include "faillog.h"
#include "fs/mkstemp/fmkomstemp.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#include "run_part.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
#include "shadowio.h"
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif				/* WITH_SELINUX */
#ifdef ENABLE_SUBIDS
#include "subordinateio.h"
#endif				/* ENABLE_SUBIDS */
#ifdef WITH_TCB
#include "tcbfuncs.h"
#endif
#include "shadow/gshadow/sgrp.h"
#include "shadowlog.h"
#include "sssd.h"
#include "string/memset/memzero.h"
#include "string/sprintf/aprintf.h"
#include "string/sprintf/snprintf.h"
#include "string/strcmp/strcaseeq.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strdup/strdup.h"
#include "string/strerrno.h"
#include "string/strtok/stpsep.h"


#ifndef SKEL_DIR
#define SKEL_DIR "/etc/skel"
#endif
#ifndef USRSKELDIR
#define USRSKELDIR "/usr/etc/skel"
#endif
#ifndef USER_DEFAULTS_FILE
#define USER_DEFAULTS_FILE "/etc/default/useradd"
#define NEW_USER_FILE "/etc/default/nuaddXXXXXX"
#endif

/*
 * Structures
 */
struct option_flags {
	bool chroot;
	bool prefix;
};

/*
 * Global variables
 */
static const char Prog[] = "useradd";

/*
 * These defaults are used if there is no defaults file.
 */
static gid_t def_group = 1000;
static const char *def_groups = "";
static const char *def_gname = "other";
static const char *def_home = "/home";
static const char *def_shell = "/bin/bash";
static const char *def_template = SKEL_DIR;
static const char *def_usrtemplate = USRSKELDIR;
static const char *def_create_mail_spool = "yes";
static const char *def_log_init = "yes";

static long def_inactive = -1;
static const char *def_expire = "";

#define VALID(s)  (!strpbrk(s, ":\n"))

static const char *user_name = "";
static const char *user_pass = "!";
static uid_t user_id;
static gid_t user_gid;
static const char *user_comment = "";
static const char *user_home = "";
static const char *user_shell = "";
static const char *create_mail_spool = "";

static const char *prefix = "";
static const char *prefix_user_home = NULL;

#ifdef WITH_SELINUX
static /*@notnull@*/const char *user_selinux = "";
static const char *user_selinux_range = NULL;
#endif				/* WITH_SELINUX */

static long user_expire = -1;
static bool is_shadow_pwd;

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
#ifdef ENABLE_SUBIDS
static bool is_sub_uid = false;
static bool is_sub_gid = false;
static bool sub_uid_locked = false;
static bool sub_gid_locked = false;
static uid_t sub_uid_start;	/* New subordinate uid range */
static gid_t sub_gid_start;	/* New subordinate gid range */
#endif				/* ENABLE_SUBIDS */
static bool pw_locked = false;
static bool gr_locked = false;
static bool spw_locked = false;
static char **user_groups;	/* NULL-terminated list */
static long sys_ngroups;
static bool do_grp_update = false;	/* group files need to be updated */

extern int allow_bad_names;

static bool
    bflg = false,		/* new default root of home directory */
    cflg = false,		/* comment (GECOS) field for new account */
    dflg = false,		/* home directory for new account */
    Dflg = false,		/* set/show new user default values */
    eflg = false,		/* days since 1970-01-01 when account is locked */
    fflg = false,		/* days until account with expired password is locked */
#ifdef ENABLE_SUBIDS
    Fflg = false,		/* update /etc/subuid and /etc/subgid even if -r option is given */
#endif
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
    subvolflg = false,		/* create subvolume home on BTRFS */
    uflg = false,		/* specify user ID for new account */
    Uflg = false;		/* create a group having the same name as the user */

#ifdef WITH_SELINUX
#define Zflg  (!streq(user_selinux, ""))
#endif				/* WITH_SELINUX */

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
#define E_NAME_IN_USE	9	/* username or group name already in use */
#define E_GRP_UPDATE	10	/* can't update group file */
#define E_HOMEDIR	12	/* can't create home directory */
#define E_MAILBOXFILE	13	/* can't create mailbox file */
#define E_SE_UPDATE	14	/* can't update SELinux user mapping */
#ifdef ENABLE_SUBIDS
#define E_SUB_UID_UPDATE 16	/* can't update the subordinate uid file */
#define E_SUB_GID_UPDATE 18	/* can't update the subordinate gid file */
#endif				/* ENABLE_SUBIDS */
#define E_BAD_NAME	19	/* Bad login name */

#define DGROUP			"GROUP"
#define DGROUPS			"GROUPS"
#define DHOME			"HOME"
#define DSHELL			"SHELL"
#define DINACT			"INACTIVE"
#define DEXPIRE			"EXPIRE"
#define DSKEL			"SKEL"
#define DUSRSKEL		"USRSKEL"
#define DCREATE_MAIL_SPOOL	"CREATE_MAIL_SPOOL"
#define DLOG_INIT		"LOG_INIT"

/* local function prototypes */
NORETURN static void fail_exit (int, bool);
static void get_defaults (struct option_flags *);
static void show_defaults (void);
static int set_defaults (void);
static int get_groups (char *, struct option_flags *);
static struct group * get_local_group (char * grp_name, bool process_selinux);
NORETURN static void usage (int status);
static void new_pwent (struct passwd *);

static void new_spent (struct spwd *);
static void grp_update (bool);

static void process_flags (int argc, char **argv, struct option_flags *flags);
static void close_files (struct option_flags *flags);
static void close_group_files (bool process_selinux);
static void unlock_group_files (bool process_selinux);
static void open_files (bool process_selinux);
static void open_group_files (bool process_selinux);
static void open_shadow (bool process_selinux);
static void faillog_reset (uid_t);
#ifdef ENABLE_LASTLOG
static void lastlog_reset (uid_t);
#endif /* ENABLE_LASTLOG */
static void tallylog_reset (const char *);
static void usr_update (unsigned long subuid_count, unsigned long subgid_count,
                        struct option_flags *flags);
static void create_home (struct option_flags *flags);
static void create_mail (struct option_flags *flags);
static void check_uid_range(int rflg, uid_t user_id);


/*
 * fail_exit - undo as much as possible
 */
static void fail_exit (int code, bool process_selinux)
{
#ifdef WITH_AUDIT
	int type;
#endif

	if (home_added && rmdir(prefix_user_home) != 0) {
		fprintf(stderr,
		        _("%s: %s was created, but could not be removed\n"),
		        Prog, prefix_user_home);
		SYSLOG((LOG_ERR, "failed to remove %s", prefix_user_home));
	}

	if (spw_locked && spw_unlock(process_selinux) == 0) {
		fprintf(stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname());
		SYSLOG((LOG_ERR, "failed to unlock %s", spw_dbname()));
		/* continue */
	}
	if (pw_locked && pw_unlock(process_selinux) == 0) {
		fprintf(stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname());
		SYSLOG((LOG_ERR, "failed to unlock %s", pw_dbname()));
		/* continue */
	}
	if (gr_locked && gr_unlock(process_selinux) == 0) {
		fprintf(stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname());
		SYSLOG((LOG_ERR, "failed to unlock %s", gr_dbname()));
		/* continue */
	}
#ifdef SHADOWGRP
	if (sgr_locked && sgr_unlock(process_selinux) == 0) {
		fprintf(stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname());
		SYSLOG((LOG_ERR, "failed to unlock %s", sgr_dbname()));
		/* continue */
	}
#endif
#ifdef ENABLE_SUBIDS
	if (sub_uid_locked && sub_uid_unlock(process_selinux) == 0) {
		fprintf(stderr, _("%s: failed to unlock %s\n"), Prog, sub_uid_dbname());
		SYSLOG((LOG_ERR, "failed to unlock %s", sub_uid_dbname()));
		/* continue */
	}
	if (sub_gid_locked && sub_gid_unlock(process_selinux) == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sub_gid_dbname());
		SYSLOG ((LOG_ERR, "failed to unlock %s", sub_gid_dbname()));
		/* continue */
	}
#endif  /* ENABLE_SUBIDS */

#ifdef WITH_AUDIT
	if (code == E_PW_UPDATE || code >= E_GRP_UPDATE)
		type = AUDIT_USER_MGMT;
	else
		type = AUDIT_ADD_USER;

	audit_logger (type,
	              "add-user",
	             user_name, AUDIT_NO_ID, SHADOW_AUDIT_FAILURE);
#endif
	SYSLOG((LOG_INFO, "failed adding user '%s', exit code: %d", user_name, code));
	exit(code);
}

/*
 * get_defaults - read the defaults file
 *
 *	get_defaults() reads the defaults file for this command. It sets the
 *	various values from the file, or uses built-in default values if the
 *	file does not exist.
 */
static void
get_defaults(struct option_flags *flags)
{
	FILE        *fp;
	char        *default_file = USER_DEFAULTS_FILE;
	char        buf[1024];
	char        *cp;
	const char  *ccp;

	if (prefix[0]) {
		default_file = aprintf("%s/%s", prefix, USER_DEFAULTS_FILE);
		if (default_file == NULL)
			return;
	}

	/*
	 * Open the defaults file for reading.
	 */

	fp = fopen (default_file, "r");
	if (NULL == fp) {
		goto getdef_err;
	}

	/*
	 * Read the file a line at a time. Only the lines that have relevant
	 * values are used, everything else can be ignored.
	 */
	while (fgets (buf, sizeof buf, fp) == buf) {
		stpsep(buf, "\n");

		cp = stpsep(buf, "=");
		if (NULL == cp)
			continue;

		/*
		 * Primary GROUP identifier
		 */
		if (streq(buf, DGROUP)) {
			const struct group *grp = prefix_getgr_nam_gid (cp);
			if (NULL == grp) {
				fprintf (stderr,
				         _("%s: group '%s' does not exist\n"),
				         Prog, cp);
				fprintf (stderr,
				         _("%s: the %s= configuration in %s will be ignored\n"),
				         Prog, DGROUP, default_file);
			} else {
				def_group = grp->gr_gid;
				def_gname = xstrdup (grp->gr_name);
			}
		}

		ccp = cp;

		if (streq(buf, DGROUPS)) {
			if (get_groups (cp, flags) != 0) {
				fprintf (stderr,
				         _("%s: the '%s=' configuration in %s has an invalid group, ignoring the bad group\n"),
				         Prog, DGROUPS, default_file);
			}
			if (user_groups[0] != NULL) {
				do_grp_update = true;
				def_groups = xstrdup (cp);
			}
		}
		/*
		 * Default HOME filesystem
		 */
		else if (streq(buf, DHOME)) {
			def_home = xstrdup(ccp);
		}

		/*
		 * Default Login Shell command
		 */
		else if (streq(buf, DSHELL)) {
			def_shell = xstrdup(ccp);
		}

		/*
		 * Default Password Inactive value
		 */
		else if (streq(buf, DINACT)) {
			if (a2sl(&def_inactive, ccp, NULL, 0, -1, LONG_MAX) == -1) {
				fprintf (stderr,
				         _("%s: invalid numeric argument '%s'\n"),
				         Prog, ccp);
				fprintf (stderr,
				         _("%s: the %s= configuration in %s will be ignored\n"),
				         Prog, DINACT, default_file);
				def_inactive = -1;
			}
		}

		/*
		 * Default account expiration date
		 */
		else if (streq(buf, DEXPIRE)) {
			def_expire = xstrdup(ccp);
		}

		/*
		 * Default Skeleton information
		 */
		else if (streq(buf, DSKEL)) {
			if (streq(ccp, ""))
				ccp = SKEL_DIR;

			if (prefix[0])
				def_template = xaprintf("%s/%s", prefix, ccp);
			else
				def_template = xstrdup(ccp);
		}

		/*
		 * Default Usr Skeleton information
		 */
		else if (streq(buf, DUSRSKEL)) {
			if (streq(ccp, ""))
				ccp = USRSKELDIR;

			if (prefix[0]) {
				def_usrtemplate = xaprintf("%s/%s", prefix, ccp);
			} else {
				def_usrtemplate = xstrdup(ccp);
			}
		}
		/*
		 * Create by default user mail spool or not ?
		 */
		else if (streq(buf, DCREATE_MAIL_SPOOL)) {
			if (streq(ccp, ""))
				ccp = "no";

			def_create_mail_spool = xstrdup(ccp);
		}

		/*
		 * By default do we add the user to the lastlog and faillog databases ?
		 */
		else if (streq(buf, DLOG_INIT)) {
			if (streq(ccp, ""))
				ccp = def_log_init;

			def_log_init = xstrdup(ccp);
		}
	}
	(void) fclose (fp);
     getdef_err:
	if (prefix[0]) {
		free(default_file);
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
	printf ("GROUPS=%s\n", def_groups);
	printf ("HOME=%s\n", def_home);
	printf ("INACTIVE=%ld\n", def_inactive);
	printf ("EXPIRE=%s\n", def_expire);
	printf ("SHELL=%s\n", def_shell);
	printf ("SKEL=%s\n", def_template);
	printf ("USRSKEL=%s\n", def_usrtemplate);
	printf ("CREATE_MAIL_SPOOL=%s\n", def_create_mail_spool);
	printf ("LOG_INIT=%s\n", def_log_init);
}

/*
 * set_defaults - write new defaults file
 *
 *	set_defaults() re-writes the defaults file using the values that
 *	are currently set. Duplicated lines are pruned, missing lines are
 *	added, and unrecognized lines are copied as is.
 */
static int
set_defaults(void)
{
	int   ret = -1;
	bool  out_group = false;
	bool  out_groups = false;
	bool  out_home = false;
	bool  out_inactive = false;
	bool  out_expire = false;
	bool  out_shell = false;
	bool  out_skel = false;
	bool  out_usrskel = false;
	bool  out_create_mail_spool = false;
	bool  out_log_init = false;
	char  buf[1024];
	char  *new_file = NULL;
	char  *new_file_dup = NULL;
	char  *default_file = USER_DEFAULTS_FILE;
	FILE  *ifp;
	FILE  *ofp;


	new_file = aprintf("%s%s%s", prefix, prefix[0]?"/":"", NEW_USER_FILE);
	if (new_file == NULL) {
		fprintf(stderr, _("%s: cannot create new defaults file: %s\n"),
		        Prog, strerrno());
		return -1;
        }

	if (prefix[0]) {
		default_file = aprintf("%s/%s", prefix, USER_DEFAULTS_FILE);
		if (default_file == NULL) {
			fprintf(stderr,
			        _("%s: cannot create new defaults file: %s\n"),
			        Prog, strerrno());
			goto err_free_new;
		}
	}

	new_file_dup = strdup(new_file);
	if (new_file_dup == NULL) {
		fprintf (stderr,
			_("%s: cannot create directory for defaults file\n"),
			Prog);
		goto err_free_def;
	}

	ret = mkdir(dirname(new_file_dup), 0755);
	free(new_file_dup);
	if (-1 == ret && EEXIST != errno) {
		fprintf (stderr,
			_("%s: cannot create directory for defaults file\n"),
			Prog);
		goto err_free_def;
	}

	/*
	 * Create a temporary file to copy the new output to.
	 */
	ofp = fmkomstemp(new_file, 0, 0644);
	if (NULL == ofp) {
		fprintf (stderr,
		         _("%s: cannot open new defaults file\n"),
		         Prog);
		goto err_free_def;
	}

	/*
	 * Open the existing defaults file and copy the lines to the
	 * temporary file, using any new values. Each line is checked
	 * to insure that it is not output more than once.
	 */
	ifp = fopen (default_file, "r");
	if (NULL == ifp) {
		fprintf (ofp, "# useradd defaults file\n");
		goto skip;
	}

	while (fgets (buf, sizeof buf, ifp) == buf) {
		char  *val;

		if (stpsep(buf, "\n") == NULL) {
			/* A line which does not end with \n is only valid
			 * at the end of the file.
			 */
			if (feof (ifp) == 0) {
				fprintf (stderr,
				         _("%s: line too long in %s: %s..."),
				         Prog, default_file, buf);
				fclose(ifp);
				fclose(ofp);
				goto err_free_def;
			}
		}

		val = stpsep(buf, "=");
		if (val == NULL) {
			fprintf(ofp, "%s\n", buf);
		} else if (!out_group && streq(buf, DGROUP)) {
			fprintf(ofp, DGROUP "=%u\n", (unsigned int) def_group);
			out_group = true;
		} else if (!out_groups && streq(buf, DGROUPS)) {
			fprintf(ofp, DGROUPS "=%s\n", def_groups);
			out_groups = true;
		} else if (!out_home && streq(buf, DHOME)) {
			fprintf(ofp, DHOME "=%s\n", def_home);
			out_home = true;
		} else if (!out_inactive && streq(buf, DINACT)) {
			fprintf(ofp, DINACT "=%ld\n", def_inactive);
			out_inactive = true;
		} else if (!out_expire && streq(buf, DEXPIRE)) {
			fprintf(ofp, DEXPIRE "=%s\n", def_expire);
			out_expire = true;
		} else if (!out_shell && streq(buf, DSHELL)) {
			fprintf(ofp, DSHELL "=%s\n", def_shell);
			out_shell = true;
		} else if (!out_skel && streq(buf, DSKEL)) {
			fprintf(ofp, DSKEL "=%s\n", def_template);
			out_skel = true;
		} else if (!out_usrskel && streq(buf, DUSRSKEL)) {
			fprintf(ofp, DUSRSKEL "=%s\n", def_usrtemplate);
			out_usrskel = true;
		} else if (!out_create_mail_spool
			   && streq(buf, DCREATE_MAIL_SPOOL))
		{
			fprintf(ofp,
			        DCREATE_MAIL_SPOOL "=%s\n",
			        def_create_mail_spool);
			out_create_mail_spool = true;
		} else if (!out_log_init && streq(buf, DLOG_INIT)) {
			fprintf(ofp, DLOG_INIT "=%s\n", def_log_init);
			out_log_init = true;
		} else {
			fprintf(ofp, "%s=%s\n", buf, val);
		}
	}
	(void) fclose (ifp);

      skip:
	/*
	 * Check each line to insure that every line was output. This
	 * causes new values to be added to a file which did not previously
	 * have an entry for that value.
	 */
	if (!out_group)
		fprintf (ofp, DGROUP "=%u\n", (unsigned int) def_group);
	if (!out_groups)
		fprintf (ofp, DGROUPS "=%s\n", def_groups);
	if (!out_home)
		fprintf (ofp, DHOME "=%s\n", def_home);
	if (!out_inactive)
		fprintf (ofp, DINACT "=%ld\n", def_inactive);
	if (!out_expire)
		fprintf (ofp, DEXPIRE "=%s\n", def_expire);
	if (!out_shell)
		fprintf (ofp, DSHELL "=%s\n", def_shell);
	if (!out_skel)
		fprintf (ofp, DSKEL "=%s\n", def_template);
	if (!out_usrskel)
		fprintf (ofp, DUSRSKEL "=%s\n", def_usrtemplate);

	if (!out_create_mail_spool)
		fprintf (ofp, DCREATE_MAIL_SPOOL "=%s\n", def_create_mail_spool);
	if (!out_log_init)
		fprintf (ofp, DLOG_INIT "=%s\n", def_log_init);
	/*
	 * Flush and close the file. Check for errors to make certain
	 * the new file is intact.
	 */
	(void) fflush (ofp);
	if (   (ferror (ofp) != 0)
	    || (fsync (fileno (ofp)) != 0)
	    || (fclose (ofp) != 0))
	{
		unlink (new_file);
		goto err_free_def;
	}

	/*
	 * Rename the current default file to its backup name.
	 */
	assert(SNPRINTF(buf, "%s-", default_file) != -1);
	unlink (buf);
	if ((link (default_file, buf) != 0) && (ENOENT != errno)) {
		fprintf (stderr,
		         _("%s: Cannot create backup file (%s): %s\n"),
		         Prog, buf, strerrno());
		unlink (new_file);
		goto err_free_def;
	}

	/*
	 * Rename the new default file to its correct name.
	 */
	if (rename (new_file, default_file) != 0) {
		fprintf (stderr,
		         _("%s: rename: %s: %s\n"),
		         Prog, new_file, strerrno());
		goto err_free_def;
	}
#ifdef WITH_AUDIT
	audit_logger (AUDIT_USYS_CONFIG,
	              "changing-useradd-defaults",
	              NULL, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
	SYSLOG ((LOG_INFO,
	         "useradd defaults: GROUP=%u, HOME=%s, SHELL=%s, INACTIVE=%ld, "
	         "EXPIRE=%s, SKEL=%s, CREATE_MAIL_SPOOL=%s, LOG_INIT=%s",
	         (unsigned int) def_group, def_home, def_shell,
	         def_inactive, def_expire, def_template,
	         def_create_mail_spool, def_log_init));
	ret = 0;

err_free_def:
	if (prefix[0])
		free(default_file);
err_free_new:
	free(new_file);

	return ret;
}

/*
 * get_groups - convert a list of group names to an array of group IDs
 *
 *	get_groups() takes a comma-separated list of group names and
 *	converts it to a NULL-terminated array. Any unknown group
 *	names are reported as errors.
 */
static int get_groups (char *list, struct option_flags *flags)
{
	struct group *grp;
	bool errors = false;
	int ngroups = 0;
	bool process_selinux;

	process_selinux = !flags->chroot && !flags->prefix;

	/*
	 * Free previous group list before creating a new one.
	 */
	free_list(user_groups);

	if (streq(list, "")) {
		return 0;
	}

	/*
	 * Open the group files
	 */
	open_group_files (process_selinux);

	/*
	 * So long as there is some data to be converted, strip off
	 * each name and look it up. A mix of numerical and string
	 * values for group identifiers is permitted.
	 */
	while (NULL != list) {
		char  *g;

		/*
		 * Strip off a single name from the list
		 */
		g = strsep(&list, ",");

		/*
		 * Names starting with digits are treated as numerical
		 * GID values, otherwise the string is looked up as is.
		 */
		grp = get_local_group(g, process_selinux);

		/*
		 * There must be a match, either by GID value or by
		 * string name.
		 * FIXME: It should exist according to gr_locate,
		 *        otherwise, we can't change its members
		 */
		if (NULL == grp) {
			fprintf (stderr,
			         _("%s: group '%s' does not exist\n"),
			         Prog, g);
			errors = true;
		}

		/*
		 * If the group doesn't exist, don't dump core...
		 * Instead, try the next one.  --marekm
		 */
		if (NULL == grp) {
			continue;
		}

		if (ngroups == sys_ngroups) {
			fprintf (stderr,
			         _("%s: too many groups specified (max %d).\n"),
			         Prog, ngroups);
			gr_free(grp);
			break;
		}

		/*
		 * Add the group name to the user's list of groups.
		 */
		user_groups[ngroups++] = xstrdup (grp->gr_name);
		gr_free (grp);
	}

	close_group_files (process_selinux);
	unlock_group_files (process_selinux);

	user_groups[ngroups] = NULL;

	/*
	 * Any errors in finding group names are fatal
	 */
	if (errors) {
		return -1;
	}

	return 0;
}

/*
 * get_local_group - checks if a given group name exists locally
 *
 *	get_local_group() checks if a given group name exists locally.
 *	If the name exists the group information is returned, otherwise NULL is
 *	returned.
 */
static struct group * get_local_group(char * grp_name, bool process_selinux)
{
	gid_t               gid;
	struct group        *result_grp = NULL;
	const struct group  *grp;

	if (get_gid(grp_name, &gid) == 0)
		grp = gr_locate_gid(gid);
	else
		grp = gr_locate(grp_name);

	if (grp != NULL) {
		result_grp = __gr_dup (grp);
		if (NULL == result_grp) {
			fprintf (stderr,
					_("%s: Out of memory. Cannot find group '%s'.\n"),
					Prog, grp_name);
			fail_exit (E_GRP_UPDATE, process_selinux);
		}
	}

	return result_grp;
}

/*
 * usage - display usage message and exit
 */
static void usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options] LOGIN\n"
	                  "       %s -D\n"
	                  "       %s -D [options]\n"
	                  "\n"
	                  "Options:\n"),
	                Prog, Prog, Prog);
	(void) fputs (_("      --badname                 do not check for bad names\n"), usageout);
	(void) fputs (_("  -b, --base-dir BASE_DIR       base directory for the home directory of the\n"
	                "                                new account\n"), usageout);
#ifdef WITH_BTRFS
	(void) fputs (_("      --btrfs-subvolume-home    use BTRFS subvolume for home directory\n"), usageout);
#endif
	(void) fputs (_("  -c, --comment COMMENT         GECOS field of the new account\n"), usageout);
	(void) fputs (_("  -d, --home-dir HOME_DIR       home directory of the new account\n"), usageout);
	(void) fputs (_("  -D, --defaults                print or change default useradd configuration\n"), usageout);
	(void) fputs (_("  -e, --expiredate EXPIRE_DATE  expiration date of the new account\n"), usageout);
	(void) fputs (_("  -f, --inactive INACTIVE       password inactivity period of the new account\n"), usageout);
#ifdef ENABLE_SUBIDS
	(void) fputs (_("  -F, --add-subids-for-system   add entries to sub[ud]id even when adding a system user\n"), usageout);
#endif
	(void) fputs (_("  -g, --gid GROUP               name or ID of the primary group of the new\n"
	                "                                account\n"), usageout);
	(void) fputs (_("  -G, --groups GROUPS           list of supplementary groups of the new\n"
	                "                                account\n"), usageout);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -k, --skel SKEL_DIR           use this alternative skeleton directory\n"), usageout);
	(void) fputs (_("  -K, --key KEY=VALUE           override /etc/login.defs defaults\n"), usageout);
#ifdef ENABLE_LASTLOG
	(void) fputs (_("  -l, --no-log-init             do not add the user to the lastlog and\n"
	                "                                faillog databases\n"), usageout);
#endif /* ENABLE_LASTLOG */
	(void) fputs (_("  -m, --create-home             create the user's home directory\n"), usageout);
	(void) fputs (_("  -M, --no-create-home          do not create the user's home directory\n"), usageout);
	(void) fputs (_("  -N, --no-user-group           do not create a group with the same name as\n"
	                "                                the user\n"), usageout);
	(void) fputs (_("  -o, --non-unique              allow to create users with duplicate\n"
	                "                                (non-unique) UID\n"), usageout);
	(void) fputs (_("  -p, --password PASSWORD       encrypted password of the new account\n"), usageout);
	(void) fputs (_("  -r, --system                  create a system account\n"), usageout);
	(void) fputs (_("  -R, --root CHROOT_DIR         directory to chroot into\n"), usageout);
	(void) fputs (_("  -P, --prefix PREFIX_DIR       prefix directory where are located the /etc/* files\n"), usageout);
	(void) fputs (_("  -s, --shell SHELL             login shell of the new account\n"), usageout);
	(void) fputs (_("  -u, --uid UID                 user ID of the new account\n"), usageout);
	(void) fputs (_("  -U, --user-group              create a group with the same name as the user\n"), usageout);
#ifdef WITH_SELINUX
	(void) fputs (_("  -Z, --selinux-user SEUSER     use a specific SEUSER for the SELinux user mapping\n"), usageout);
	(void) fputs (_("      --selinux-range SERANGE   use a specific MLS range for the SELinux user mapping\n"), usageout);
#endif				/* WITH_SELINUX */
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
	spent->sp_lstchg = gettime () / DAY;
	if (0 == spent->sp_lstchg) {
		/* Better disable aging than requiring a password change */
		spent->sp_lstchg = -1;
	}
	if (!rflg) {
		spent->sp_min = getdef_num ("PASS_MIN_DAYS", -1);
		spent->sp_max = getdef_num ("PASS_MAX_DAYS", -1);
		spent->sp_warn = getdef_num ("PASS_WARN_AGE", -1);
		spent->sp_inact = def_inactive;
		spent->sp_expire = user_expire;
	} else {
		spent->sp_min = -1;
		spent->sp_max = -1;
		spent->sp_warn = -1;
		spent->sp_inact = -1;
		spent->sp_expire = -1;
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
static void grp_update (bool process_selinux)
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
	 * FIXME: we currently do not check that all groups of user_groups
	 *        were completed with the new user.
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
			fail_exit (E_GRP_UPDATE, process_selinux);	/* XXX */
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
			fail_exit (E_GRP_UPDATE, process_selinux);
		}
#ifdef WITH_AUDIT
		audit_logger_with_group (AUDIT_USER_MGMT,
		              "add-user-to-group",
		              user_name, AUDIT_NO_ID, "grp", ngrp->gr_name,
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
		 * FIXME: is it really needed?
		 *        This would be important only if the group is in
		 *        user_groups. All these groups should be checked
		 *        for existence with gr_locate already.
		 */
		if (gr_locate (sgrp->sg_namp) == NULL) {
			continue;
		}

		if (!is_on_list (user_groups, sgrp->sg_namp)) {
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
			fail_exit (E_GRP_UPDATE, process_selinux);	/* XXX */
		}

		/*
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */
		nsgrp->sg_mem = add_list (nsgrp->sg_mem, user_name);
		if (sgr_update (nsgrp) == 0) {
			fprintf (stderr,
			         _("%s: failed to prepare the new %s entry '%s'\n"),
			         Prog, sgr_dbname (), nsgrp->sg_namp);
			SYSLOG ((LOG_ERR, "failed to prepare the new %s entry '%s'", sgr_dbname (), user_name));

			fail_exit (E_GRP_UPDATE, process_selinux);
		}
#ifdef WITH_AUDIT
		audit_logger_with_group (AUDIT_USER_MGMT,
		              "add-to-shadow-group",
		              user_name, AUDIT_NO_ID, "grp", nsgrp->sg_namp,
		              SHADOW_AUDIT_SUCCESS);
#endif
		SYSLOG ((LOG_INFO,
		         "add '%s' to shadow group '%s'",
		         user_name, nsgrp->sg_namp));
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
static void process_flags (int argc, char **argv, struct option_flags *flags)
{
	const struct group *grp;
	bool anyflag = false;
	char *cp;
	struct stat st;

	{
		/*
		 * Parse the command line options.
		 */
		int c;
		static struct option long_options[] = {
			{"base-dir",       required_argument, NULL, 'b'},
#ifdef WITH_BTRFS
			{"btrfs-subvolume-home", no_argument, NULL, 200},
#endif
			{"badname",        no_argument,       NULL, 201},
			{"comment",        required_argument, NULL, 'c'},
			{"home-dir",       required_argument, NULL, 'd'},
			{"defaults",       no_argument,       NULL, 'D'},
			{"expiredate",     required_argument, NULL, 'e'},
			{"inactive",       required_argument, NULL, 'f'},
#ifdef ENABLE_SUBIDS
			{"add-subids-for-system", no_argument,NULL, 'F'},
#endif
			{"gid",            required_argument, NULL, 'g'},
			{"groups",         required_argument, NULL, 'G'},
			{"help",           no_argument,       NULL, 'h'},
			{"skel",           required_argument, NULL, 'k'},
			{"key",            required_argument, NULL, 'K'},
			{"no-log-init",    no_argument,       NULL, 'l'},
			{"create-home",    no_argument,       NULL, 'm'},
			{"no-create-home", no_argument,       NULL, 'M'},
			{"no-user-group",  no_argument,       NULL, 'N'},
			{"non-unique",     no_argument,       NULL, 'o'},
			{"password",       required_argument, NULL, 'p'},
			{"system",         no_argument,       NULL, 'r'},
			{"root",           required_argument, NULL, 'R'},
			{"prefix",         required_argument, NULL, 'P'},
			{"shell",          required_argument, NULL, 's'},
			{"uid",            required_argument, NULL, 'u'},
			{"user-group",     no_argument,       NULL, 'U'},
#ifdef WITH_SELINUX
			{"selinux-user",   required_argument, NULL, 'Z'},
			{"selinux-range",  required_argument, NULL, 202},
#endif				/* WITH_SELINUX */
			{NULL, 0, NULL, '\0'}
		};
		while ((c = getopt_long (argc, argv,
					 "b:c:d:De:f:g:G:hk:K:lmMNop:rR:P:s:u:U"
#ifdef WITH_SELINUX
		                         "Z:"
#endif				/* WITH_SELINUX */
#ifdef ENABLE_SUBIDS
		                         "F"
#endif				/* ENABLE_SUBIDS */
					 "",
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
			case 200:
				subvolflg = true;
				break;
			case 201:
				allow_bad_names = true;
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
				if (!streq(optarg, "")) {
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
				 * -e "" is allowed without /etc/shadow
				 * (it's a no-op in such case)
				 */
				if ((-1 != user_expire) && !is_shadow_pwd) {
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
				if (a2sl(&def_inactive, optarg, NULL, 0, -1, LONG_MAX)
				    == -1)
				{
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
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
#ifdef ENABLE_SUBIDS
			case 'F':
				Fflg = true;
				break;
#endif
			case 'g':
				grp = prefix_getgr_nam_gid (optarg);
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
				if (get_groups (optarg, flags) != 0) {
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
				cp = stpsep(optarg, "=");
				if (NULL == cp) {
					fprintf (stderr,
					         _("%s: -K requires KEY=VALUE\n"),
					         Prog);
					exit (E_BAD_ARG);
				}
				if (putdef_str (optarg, cp, NULL) < 0) {
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
			case 'R': /* no-op, handled in process_root_flag () */
				flags->chroot = true;
				break;
			case 'P': /* no-op, handled in process_prefix_flag () */
				flags->prefix = true;
				break;
			case 's':
				if (   ( !VALID (optarg) )
				    || (   !streq(optarg, "")
				        && ('/'  != optarg[0])
				        && ('*'  != optarg[0]) )) {
					fprintf (stderr,
					         _("%s: invalid shell '%s'\n"),
					         Prog, optarg);
					exit (E_BAD_ARG);
				}
				if (!streq(optarg, "")
				     && '*'  != optarg[0]
				     && !streq(optarg, "/sbin/nologin")
				     && !streq(optarg, "/usr/sbin/nologin")
				     && (   stat(optarg, &st) != 0
				         || S_ISDIR(st.st_mode)
				         || access(optarg, X_OK) != 0)) {
					fprintf (stderr,
					         _("%s: Warning: missing or non-executable shell '%s'\n"),
					         Prog, optarg);
				}
				user_shell = optarg;
				def_shell = optarg;
				sflg = true;
				break;
			case 'u':
				if (   (get_uid(optarg, &user_id) == -1)
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
				if (prefix[0]) {
					fprintf (stderr,
					         _("%s: -Z cannot be used with --prefix\n"),
					         Prog);
					exit (E_BAD_ARG);
				}
				if (is_selinux_enabled () > 0) {
					user_selinux = optarg;
				} else {
					fprintf (stderr,
					         _("%s: -Z requires SELinux enabled kernel\n"),
					         Prog);

					exit (E_BAD_ARG);
				}
				break;
			case 202:
				user_selinux_range = optarg;
				break;
#endif				/* WITH_SELINUX */
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
#ifdef WITH_SELINUX
	if (user_selinux_range && !Zflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "--selinux-range", "--selinux-user");
		usage (E_USAGE);
	}
#endif				/* WITH_SELINUX */

	/*
	 * Either -D or username is required. Defaults can be set with -D
	 * for the -b, -e, -f, -g, -s options only.
	 */
	if (Dflg) {
		if (optind != argc) {
			usage (E_USAGE);
		}

		if (uflg || Gflg || dflg || cflg || mflg) {
			usage (E_USAGE);
		}
	} else {
		if (optind != argc - 1) {
			usage (E_USAGE);
		}

		user_name = argv[optind];
		if (!is_valid_user_name(user_name)) {
			if (errno == EINVAL) {
				fprintf(stderr,
				        _("%s: invalid user name '%s': use --badname to ignore\n"),
				        Prog, user_name);
			} else {
				fprintf(stderr,
				        _("%s: invalid user name '%s'\n"),
				        Prog, user_name);
			}
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER,
			              "add-user",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			exit (E_BAD_NAME);
		}
		if (!dflg) {
			user_home = xaprintf("%s/%s", def_home, user_name);
		}
		if (prefix[0]) {
			prefix_user_home = xaprintf("%s/%s", prefix, user_home);
		} else {
			prefix_user_home = user_home;
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

	if (!lflg) {
		/* If we are missing the flag lflg aka -l, check the defaults
		* file to see if we need to disable it as a default*/
		if (streq(def_log_init, "no")) {
			lflg = true;
		}
	}

	if (!rflg) {
		/* for system accounts defaults are ignored and we
		 * do not create a home dir */
		if (getdef_bool ("CREATE_HOME")) {
			mflg = true;
		}
	} else {
		/* Do not automatically add supplements groups for system users. */
		if (!Gflg && do_grp_update) {
			free_list(user_groups);
			do_grp_update = false;
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
static void close_files (struct option_flags *flags)
{
	bool process_selinux;

	process_selinux = !flags->chroot && !flags->prefix;

	if (pw_close (process_selinux) == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", pw_dbname ()));
		fail_exit (E_PW_UPDATE, process_selinux);
	}
	if (is_shadow_pwd && (spw_close (process_selinux) == 0)) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"), Prog, spw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", spw_dbname ()));
		fail_exit (E_PW_UPDATE, process_selinux);
	}

	close_group_files (process_selinux);

#ifdef ENABLE_SUBIDS
	if (is_sub_uid  && (sub_uid_close (process_selinux) == 0)) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"), Prog, sub_uid_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", sub_uid_dbname ()));
		fail_exit (E_SUB_UID_UPDATE, process_selinux);
	}
	if (is_sub_gid  && (sub_gid_close (process_selinux) == 0)) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"), Prog, sub_gid_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", sub_gid_dbname ()));
		fail_exit (E_SUB_GID_UPDATE, process_selinux);
	}
#endif				/* ENABLE_SUBIDS */
	if (is_shadow_pwd) {
		if (spw_unlock (process_selinux) == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER,
			              "unlocking-shadow-file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
		spw_locked = false;
	}
	if (pw_unlock (process_selinux) == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER,
		              "unlocking-passwd-file",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		/* continue */
	}
	pw_locked = false;

	unlock_group_files (process_selinux);

#ifdef ENABLE_SUBIDS
	if (is_sub_uid) {
		if (sub_uid_unlock (process_selinux) == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sub_uid_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sub_uid_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER,
				"unlocking-subordinate-user-file",
				user_name, AUDIT_NO_ID,
				SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
		sub_uid_locked = false;
	}
	if (is_sub_gid) {
		if (sub_gid_unlock (process_selinux) == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sub_gid_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sub_gid_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER,
				"unlocking-subordinate-group-file",
				user_name, AUDIT_NO_ID,
				SHADOW_AUDIT_FAILURE);
#endif
			/* continue */
		}
		sub_gid_locked = false;
	}
#endif				/* ENABLE_SUBIDS */
}

/*
 * close_group_files - close all of the files that were opened
 *
 *	close_group_files() closes all of the files that were opened related
 *  with groups. This causes any modified entries to be written out.
 */
static void close_group_files (bool process_selinux)
{
	if (!do_grp_update)
		return;

	if (gr_close(process_selinux) == 0) {
		fprintf(stderr,
		        _("%s: failure while writing changes to %s\n"),
		        Prog, gr_dbname());
		SYSLOG((LOG_ERR, "failure while writing changes to %s", gr_dbname()));
		fail_exit(E_GRP_UPDATE, process_selinux);
	}
#ifdef	SHADOWGRP
	if (is_shadow_grp && sgr_close(process_selinux) == 0) {
		fprintf(stderr,
		        _("%s: failure while writing changes to %s\n"),
		        Prog, sgr_dbname());
		SYSLOG((LOG_ERR, "failure while writing changes to %s", sgr_dbname()));
		fail_exit(E_GRP_UPDATE, process_selinux);
	}
#endif /* SHADOWGRP */
}

/*
 * unlock_group_files - unlock all of the files that were locked
 *
 *	unlock_group_files() unlocks all of the files that were locked related
 *  with groups. This causes any modified entries to be written out.
 */
static void unlock_group_files (bool process_selinux)
{
	if (gr_unlock (process_selinux) == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_USER,
		              "unlocking-group-file",
		              user_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif /* WITH_AUDIT */
		/* continue */
	}
	gr_locked = false;
#ifdef	SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_unlock (process_selinux) == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ADD_USER,
			              "unlocking-gshadow-file",
			              user_name, AUDIT_NO_ID,
			              SHADOW_AUDIT_FAILURE);
#endif /* WITH_AUDIT */
			/* continue */
		}
		sgr_locked = false;
	}
#endif /* SHADOWGRP */
}

/*
 * open_files - lock and open the password files
 *
 *	open_files() opens the two password files.
 */
static void open_files (bool process_selinux)
{
	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
		exit (E_PW_UPDATE);
	}
	pw_locked = true;
	if (pw_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, pw_dbname ());
		fail_exit (E_PW_UPDATE, process_selinux);
	}

	/* shadow file will be opened by open_shadow(); */

	open_group_files (process_selinux);

#ifdef ENABLE_SUBIDS
	if (is_sub_uid) {
		if (sub_uid_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sub_uid_dbname ());
			fail_exit (E_SUB_UID_UPDATE, process_selinux);
		}
		sub_uid_locked = true;
		if (sub_uid_open (O_CREAT | O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sub_uid_dbname ());
			fail_exit (E_SUB_UID_UPDATE, process_selinux);
		}
	}
	if (is_sub_gid) {
		if (sub_gid_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sub_gid_dbname ());
			fail_exit (E_SUB_GID_UPDATE, process_selinux);
		}
		sub_gid_locked = true;
		if (sub_gid_open (O_CREAT | O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sub_gid_dbname ());
			fail_exit (E_SUB_GID_UPDATE, process_selinux);
		}
	}
#endif				/* ENABLE_SUBIDS */
}

static void open_group_files (bool process_selinux)
{
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
		fail_exit (E_GRP_UPDATE, process_selinux);
	}
	gr_locked = true;
	if (gr_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
		fail_exit (E_GRP_UPDATE, process_selinux);
	}

#ifdef  SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			fail_exit (E_GRP_UPDATE, process_selinux);
		}
		sgr_locked = true;
		if (sgr_open (O_CREAT | O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
			fail_exit (E_GRP_UPDATE, process_selinux);
		}
	}
#endif /* SHADOWGRP */
}

static void open_shadow (bool process_selinux)
{
	if (!is_shadow_pwd) {
		return;
	}
	if (spw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE, process_selinux);
	}
	spw_locked = true;
	if (spw_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"),
		         Prog, spw_dbname ());
		fail_exit (E_PW_UPDATE, process_selinux);
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
#ifdef  SHADOWGRP
	if (is_shadow_grp) {
		grent->gr_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
	} else
#endif				/* SHADOWGRP */
	{
		grent->gr_passwd = "!";	/* XXX warning: const */
	}
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
	sgent->sg_namp = (char *) user_name;
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

static void grp_add (bool process_selinux)
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
		audit_logger (AUDIT_ADD_GROUP,
		              "add-group",
		              grp.gr_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_GRP_UPDATE, process_selinux);
	}
#ifdef  SHADOWGRP
	/*
	 * Write out the new shadow group entries as well.
	 */
	if (is_shadow_grp && (sgr_update (&sgrp) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, sgr_dbname (), sgrp.sg_namp);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ADD_GROUP,
		              "add-group",
		              grp.gr_name, AUDIT_NO_ID,
		              SHADOW_AUDIT_FAILURE);
#endif
		fail_exit (E_GRP_UPDATE, process_selinux);
	}
#endif				/* SHADOWGRP */
	SYSLOG ((LOG_INFO, "new group: name=%s, GID=%u", user_name, user_gid));
#ifdef WITH_AUDIT
	audit_logger (AUDIT_ADD_GROUP,
	              "add-group",
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
	struct stat st;

	if (stat (FAILLOG_FILE, &st) != 0 || st.st_size <= offset_uid) {
		return;
	}

	memzero (&fl, sizeof (fl));

	fd = open (FAILLOG_FILE, O_RDWR);
	if (-1 == fd) {
		fprintf (stderr,
		         _("%s: failed to open the faillog file for UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to open the faillog file for UID %lu", (unsigned long) uid));
		return;
	}
	if (   (lseek (fd, offset_uid, SEEK_SET) != offset_uid)
	    || (write_full(fd, &fl, sizeof (fl)) == -1)
	    || (fsync (fd) != 0)) {
		fprintf (stderr,
		         _("%s: failed to reset the faillog entry of UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to reset the faillog entry of UID %lu", (unsigned long) uid));
	}
	if (close (fd) != 0 && errno != EINTR) {
		fprintf (stderr,
		         _("%s: failed to close the faillog file for UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to close the faillog file for UID %lu", (unsigned long) uid));
	}
}

#ifdef ENABLE_LASTLOG
static void lastlog_reset (uid_t uid)
{
	struct lastlog ll;
	int fd;
	off_t offset_uid = (off_t) (sizeof ll) * uid;
	uid_t max_uid;
	struct stat st;

	if (stat(_PATH_LASTLOG, &st) != 0 || st.st_size <= offset_uid) {
		return;
	}

	max_uid = getdef_ulong ("LASTLOG_UID_MAX", 0xFFFFFFFFUL);
	if (uid > max_uid) {
		/* do not touch lastlog for large uids */
		return;
	}

	memzero (&ll, sizeof (ll));

	fd = open(_PATH_LASTLOG, O_RDWR);
	if (-1 == fd) {
		fprintf (stderr,
		         _("%s: failed to open the lastlog file for UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to open the lastlog file for UID %lu", (unsigned long) uid));
		return;
	}
	if (   (lseek (fd, offset_uid, SEEK_SET) != offset_uid)
	    || (write_full (fd, &ll, sizeof (ll)) == -1)
	    || (fsync (fd) != 0)) {
		fprintf (stderr,
		         _("%s: failed to reset the lastlog entry of UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to reset the lastlog entry of UID %lu", (unsigned long) uid));
		/* continue */
	}
	if (close (fd) != 0 && errno != EINTR) {
		fprintf (stderr,
		         _("%s: failed to close the lastlog file for UID %lu: %s\n"),
		        Prog, (unsigned long) uid, strerrno());
		SYSLOG ((LOG_WARN, "failed to close the lastlog file for UID %lu", (unsigned long) uid));
		/* continue */
	}
}
#endif /* ENABLE_LASTLOG */

static void tallylog_reset (const char *user_name)
{
	const char pam_tally2[] = "/sbin/pam_tally2";
	const char *pname;
	pid_t childpid;
	int failed;
	int status;

	if (access(pam_tally2, X_OK) == -1)
		return;

	/* set SIGCHLD to default for waitpid */
	signal(SIGCHLD, SIG_DFL);

	failed = 0;
	switch (childpid = fork())
	{
	case -1: /* error */
		failed = 1;
		break;
	case 0: /* child */
		pname = Basename(pam_tally2);
		execl(pam_tally2, pname, "--user", user_name, "--reset", "--quiet", NULL);
		/* If we come here, something has gone terribly wrong */
		perror(pam_tally2);
		exit(42);       /* don't continue, we now have 2 processes running! */
		/* NOTREACHED */
		break;
	default: /* parent */
		if (waitpid(childpid, &status, 0) == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
			failed = 1;
		break;
	}

	if (failed)
	{
		fprintf (stderr,
		         _("%s: failed to reset the tallylog entry of user \"%s\"\n"),
		         Prog, user_name);
		SYSLOG ((LOG_WARN, "failed to reset the tallylog entry of user \"%s\"", user_name));
	}

	return;
}

/*
 * usr_update - create the user entries
 *
 *	usr_update() creates the password file entries for this user
 *	and will update the group entries if required.
 */
static void
usr_update (unsigned long subuid_count, unsigned long subgid_count,
            struct option_flags *flags)
{
	struct passwd pwent;
	struct spwd spent;
	char *tty;
	bool process_selinux;

	process_selinux = !flags->chroot && !flags->prefix;

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
	tty=ttyname (STDIN_FILENO);
	SYSLOG ((LOG_INFO,
	         "new user: name=%s, UID=%u, GID=%u, home=%s, shell=%s, from=%s",
	         user_name, (unsigned int) user_id,
	         (unsigned int) user_gid, user_home, user_shell,
	         tty ? tty : "none" ));

	/*
	 * Initialize faillog and lastlog entries for this UID in case
	 * it belongs to a previously deleted user. We do it only if
	 * no user with this UID exists yet (entries for shared UIDs
	 * are left unchanged).  --marekm
	 */
	/* local, no need for xgetpwuid */
	if ((!lflg) && (prefix_getpwuid (user_id) == NULL)) {
		faillog_reset (user_id);
#ifdef ENABLE_LASTLOG
		lastlog_reset (user_id);
#endif /* ENABLE_LASTLOG */
	}

	/*
	 * Put the new (struct passwd) in the table.
	 */
	if (pw_update (&pwent) == 0) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, pw_dbname (), pwent.pw_name);
		fail_exit (E_PW_UPDATE, process_selinux);
	}

	/*
	 * Put the new (struct spwd) in the table.
	 */
	if (is_shadow_pwd && (spw_update (&spent) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry '%s'\n"),
		         Prog, spw_dbname (), spent.sp_namp);
		fail_exit (E_PW_UPDATE, process_selinux);
	}
#ifdef ENABLE_SUBIDS
	if (is_sub_uid && !local_sub_uid_assigned(user_name) &&
	    (sub_uid_add(user_name, sub_uid_start, subuid_count) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry\n"),
		         Prog, sub_uid_dbname ());
		fail_exit (E_SUB_UID_UPDATE, process_selinux);
	}
	if (is_sub_gid && !local_sub_gid_assigned(user_name) &&
	    (sub_gid_add(user_name, sub_gid_start, subgid_count) == 0)) {
		fprintf (stderr,
		         _("%s: failed to prepare the new %s entry\n"),
		         Prog, sub_uid_dbname ());
		fail_exit (E_SUB_GID_UPDATE, process_selinux);
	}
#endif				/* ENABLE_SUBIDS */

#ifdef WITH_AUDIT
	/*
	 * Even though we have the ID of the user, we won't send it now
	 * because its not written to disk yet. After close_files it is
	 * and we can use the real ID thereafter.
	 */
	audit_logger (AUDIT_ADD_USER,
	              "add-user",
	              user_name, AUDIT_NO_ID,
	              SHADOW_AUDIT_SUCCESS);
#endif
	/*
	 * Do any group file updates for this user.
	 */
	if (do_grp_update) {
		grp_update (process_selinux);
	}
}

/*
 * create_home - create the user's home directory
 *
 *	create_home() creates the user's home directory if it does not
 *	already exist. It will be created mode 755 owned by the user
 *	with the user's default group.
 */
static void create_home (struct option_flags *flags)
{
	char    path[strlen(prefix_user_home) + 2];
	char    *bhome, *cp;
	mode_t  mode;
	bool    process_selinux;

	process_selinux = !flags->chroot && !flags->prefix;

	if (access (prefix_user_home, F_OK) == 0)
		return;

	strcpy(path, "");
	bhome = strdup(prefix_user_home);
	if (!bhome) {
		fprintf(stderr,
			_("%s: error while duplicating string %s\n"),
			Prog, user_home);
		fail_exit(E_HOMEDIR, process_selinux);
	}

#ifdef WITH_SELINUX
	if (process_selinux) {
		if (set_selinux_file_context(prefix_user_home, S_IFDIR) != 0) {
			fprintf(stderr,
				_("%s: cannot set SELinux context for home directory %s\n"),
				Prog, user_home);
			fail_exit(E_HOMEDIR, process_selinux);
		}
	}
#endif

	/* Check for every part of the path, if the directory
	   exists. If not, create it with permissions 755 and
	   owner root:root.
	 */
	for (cp = strtok(bhome, "/"); cp != NULL; cp = strtok(NULL, "/")) {
		/* Avoid turning a relative path into an absolute path. */
		if (strprefix(bhome, "/") || !streq(path, ""))
			strcat(path, "/");

		strcat(path, cp);
		if (access(path, F_OK) == 0) {
			continue;
		}

		/* Check if parent directory is BTRFS, fail if requesting
		   subvolume but no BTRFS. The paths could be different by the
		   trailing slash
		 */
#if WITH_BTRFS
		if (subvolflg && (strlen(prefix_user_home) - (int)strlen(path)) <= 1) {
			char *btrfs_check = strdup(path);

			if (!btrfs_check) {
				fprintf(stderr,
					_("%s: error while duplicating string in BTRFS check %s\n"),
					Prog, path);
				fail_exit(E_HOMEDIR, process_selinux);
			}
			stpcpy(&btrfs_check[strlen(path) - strlen(cp) - 1], "");
			if (is_btrfs(btrfs_check) <= 0) {
				fprintf(stderr,
					_("%s: home directory \"%s\" must be mounted on BTRFS\n"),
					Prog, path);
				fail_exit(E_HOMEDIR, process_selinux);
			}
			free(btrfs_check);
			// make subvolume to mount for user instead of directory
			if (btrfs_create_subvolume(path)) {
				fprintf(stderr,
					_("%s: failed to create BTRFS subvolume: %s\n"),
					Prog, path);
				fail_exit(E_HOMEDIR, process_selinux);
			}
		}
		else
#endif
		if (mkdir(path, 0) != 0) {
			fprintf(stderr, _("%s: cannot create directory %s\n"),
				Prog, path);
			fail_exit(E_HOMEDIR, process_selinux);
		}
		if (chown(path, 0, 0) < 0) {
			fprintf(stderr,
				_("%s: warning: chown on `%s' failed: %m\n"),
				Prog, path);
		}
		if (chmod(path, 0755) < 0) {
			fprintf(stderr,
				_("%s: warning: chmod on `%s' failed: %m\n"),
				Prog, path);
		}
	}
	free(bhome);

	(void) chown(prefix_user_home, user_id, user_gid);
	mode = getdef_num("HOME_MODE",
			  0777 & ~getdef_num("UMASK", GETDEF_DEFAULT_UMASK));
	if (chmod(prefix_user_home, mode)) {
		fprintf(stderr, _("%s: warning: chown on '%s' failed: %m\n"),
			Prog, path);
	}
	home_added = true;
#ifdef WITH_AUDIT
	audit_logger(AUDIT_USER_MGMT, "add-home-dir",
		     user_name, user_id, SHADOW_AUDIT_SUCCESS);
#endif
#ifdef WITH_SELINUX
	if (process_selinux) {
		/* Reset SELinux to create files with default contexts */
		if (reset_selinux_file_context() != 0) {
			fprintf(stderr,
				_("%s: cannot reset SELinux file creation context\n"),
				Prog);
			fail_exit(E_HOMEDIR, process_selinux);
		}
	}
#endif
}

/*
 * create_mail - create the user's mail spool
 *
 *	create_mail() creates the user's mail spool if it does not already
 *	exist. It will be created mode 660 owned by the user and group
 *	'mail'
 */
static void create_mail (struct option_flags *flags)
{
	int           fd;
	char          *file;
	gid_t         gid;
	mode_t        mode;
	const char    *spool;
	struct group  *gr;
	bool          process_selinux;

	process_selinux = !flags->chroot && !flags->prefix;

	if (!strcaseeq(create_mail_spool, "yes"))
		return;

	spool = getdef_str("MAIL_DIR");
#ifdef MAIL_SPOOL_DIR
	if ((NULL == spool) && (getdef_str("MAIL_FILE") == NULL)) {
		spool = MAIL_SPOOL_DIR;
	}
#endif /* MAIL_SPOOL_DIR */
	if (NULL == spool) {
		return;
	}
	if (prefix[0])
		file = xaprintf("%s/%s/%s", prefix, spool, user_name);
	else
		file = xaprintf("%s/%s", spool, user_name);

#ifdef WITH_SELINUX
	if (process_selinux) {
		if (set_selinux_file_context(file, S_IFREG) != 0) {
			fprintf(stderr,
					_("%s: cannot set SELinux context for mailbox file %s\n"),
					Prog, file);
			fail_exit(E_MAILBOXFILE, process_selinux);
		}
	}
#endif

	fd = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0);
	free(file);

	if (fd < 0) {
		perror(_("Creating mailbox file"));
		return;
	}

	gr = prefix_getgrnam("mail"); /* local, no need for xgetgrnam */
	if (NULL == gr) {
		fputs(_("Group 'mail' not found. Creating the user mailbox file with 0600 mode.\n"),
		       stderr);
		gid = user_gid;
		mode = 0600;
	} else {
		gid = gr->gr_gid;
		mode = 0660;
	}

	if (fchown(fd, user_id, gid) != 0
	 || fchmod(fd, mode) != 0)
	{
		perror(_("Setting mailbox file permissions"));
	}

	if (fsync(fd) != 0) {
		perror (_("Synchronize mailbox file"));
	}
	if (close(fd) != 0 && errno != EINTR) {
		perror (_("Closing mailbox file"));
	}
#ifdef WITH_SELINUX
	if (process_selinux) {
		/* Reset SELinux to create files with default contexts */
		if (reset_selinux_file_context() != 0) {
			fprintf(stderr,
					_("%s: cannot reset SELinux file creation context\n"),
					Prog);
			fail_exit(E_MAILBOXFILE, process_selinux);
		}
	}
#endif
}

static void check_uid_range(int rflg, uid_t user_id)
{
	uid_t uid_min ;
	uid_t uid_max ;
	if (rflg) {
		uid_max = getdef_ulong("SYS_UID_MAX",getdef_ulong("UID_MIN",1000UL)-1);
		if (user_id > uid_max) {
			fprintf(stderr, _("%s warning: %s's uid %d is greater than SYS_UID_MAX %d\n"), Prog, user_name, user_id, uid_max);
		}
	}else{
		uid_min = getdef_ulong("UID_MIN", 1000UL);
		uid_max = getdef_ulong("UID_MAX", 6000UL);
		if (uid_min <= uid_max) {
			if (user_id < uid_min || user_id >uid_max)
				fprintf(stderr, _("%s warning: %s's uid %d outside of the UID_MIN %d and UID_MAX %d range.\n"), Prog, user_name, user_id, uid_min, uid_max);
		}
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

#ifdef ENABLE_SUBIDS
	uid_t uid_min;
	uid_t uid_max;
#endif
	unsigned long subuid_count = 0;
	unsigned long subgid_count = 0;
	struct option_flags  flags;
	bool process_selinux;

	log_set_progname(Prog);
	log_set_logfd(stderr);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_root_flag ("-R", argc, argv);

	prefix = process_prefix_flag("-P", argc, argv);

	OPENLOG (Prog);
#ifdef WITH_AUDIT
	audit_help_open ();
#endif

	sys_ngroups = sysconf (_SC_NGROUPS_MAX);
	user_groups = XMALLOC(1 + sys_ngroups, char *);
	/*
	 * Initialize the list to be empty
	 */
	user_groups[0] = NULL;


	is_shadow_pwd = spw_file_present ();
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	get_defaults (&flags);

	process_flags (argc, argv, &flags);
	process_selinux = !flags.chroot && !flags.prefix;

#ifdef ENABLE_SUBIDS
	uid_min = getdef_ulong ("UID_MIN", 1000UL);
	uid_max = getdef_ulong ("UID_MAX", 60000UL);
	subuid_count = getdef_ulong ("SUB_UID_COUNT", 65536);
	subgid_count = getdef_ulong ("SUB_GID_COUNT", 65536);
	is_sub_uid = want_subuid_file () &&
	    subuid_count > 0 && sub_uid_file_present () &&
	    (!rflg || Fflg) &&
	    (!user_id || (user_id <= uid_max && user_id >= uid_min));
	is_sub_gid = want_subgid_file() &&
	    subgid_count > 0 && sub_gid_file_present() &&
	    (!rflg || Fflg) &&
	    (!user_id || (user_id <= uid_max && user_id >= uid_min));
#endif				/* ENABLE_SUBIDS */

	if (run_parts ("/etc/shadow-maint/useradd-pre.d", user_name,
			"useradd")) {
		exit(1);
	}

#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	{
		struct passwd *pampw;
		pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
		if (pampw == NULL && getuid ()) {
			fprintf (stderr,
			         _("%s: Cannot determine your user name.\n"),
			         Prog);
			fail_exit (1, process_selinux);
		}

		retval = pam_start (Prog, pampw?pampw->pw_name:"root", &conv, &pamh);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_authenticate (pamh, 0);
	}

	if (PAM_SUCCESS == retval) {
		retval = pam_acct_mgmt (pamh, 0);
	}

	if (PAM_SUCCESS != retval) {
		fprintf (stderr, _("%s: PAM: %s\n"),
		         Prog, pam_strerror (pamh, retval));
		SYSLOG((LOG_ERR, "%s", pam_strerror (pamh, retval)));
		if (NULL != pamh) {
			(void) pam_end (pamh, retval);
		}
		fail_exit (1, process_selinux);
	}
	(void) pam_end (pamh, retval);
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
	if (prefix_getpwnam (user_name) != NULL) { /* local, no need for xgetpwnam */
		fprintf (stderr, _("%s: user '%s' already exists\n"), Prog, user_name);
		fail_exit (E_NAME_IN_USE, process_selinux);
	}

	/*
	 * Don't blindly overwrite a group when a user is added...
	 * If you already have a group username, and want to add the user
	 * to that group, use useradd -g username username.
	 * --bero
	 */
	if (Uflg) {
		/* local, no need for xgetgrnam */
		if (prefix_getgrnam (user_name) != NULL) {
			fprintf (stderr,
			         _("%s: group %s exists - if you want to add this user to that group, use -g.\n"),
			         Prog, user_name);
			fail_exit (E_NAME_IN_USE, process_selinux);
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
	open_files (process_selinux);

	if (!oflg) {
		/* first, seek for a valid uid to use for this user.
		 * We do this because later we can use the uid we found as
		 * gid too ... --gafton */
		if (!uflg) {
			if (find_new_uid (rflg, &user_id, NULL) < 0) {
				fprintf (stderr, _("%s: can't create user\n"), Prog);
				fail_exit (E_UID_IN_USE, process_selinux);
			}
		} else {
			if (prefix_getpwuid (user_id) != NULL) {
				fprintf (stderr,
				         _("%s: UID %lu is not unique\n"),
				         Prog, (unsigned long) user_id);
				fail_exit (E_UID_IN_USE, process_selinux);
			}
		}
	}

	if (uflg)
	   check_uid_range(rflg,user_id);
#ifdef WITH_TCB
	if (getdef_bool ("USE_TCB")) {
		if (shadowtcb_create (user_name, user_id) == SHADOWTCB_FAILURE) {
			fprintf (stderr,
			         _("%s: Failed to create tcb directory for %s\n"),
			         Prog, user_name);
			fail_exit (E_UID_IN_USE, process_selinux);
		}
	}
#endif
	open_shadow (process_selinux);

	/* do we have to add a group for that user? This is why we need to
	 * open the group files in the open_files() function  --gafton */
	if (Uflg) {
		if (find_new_gid (rflg, &user_gid, &user_id) < 0) {
			fprintf (stderr,
			         _("%s: can't create group\n"),
			         Prog);
			fail_exit (4, process_selinux);
		}
		grp_add (process_selinux);
	}

#ifdef ENABLE_SUBIDS
	if (is_sub_uid && subuid_count != 0) {
		if (find_new_sub_uids(&sub_uid_start, &subuid_count) < 0) {
			fprintf (stderr,
			         _("%s: can't create subordinate user IDs\n"),
			         Prog);
			fail_exit(E_SUB_UID_UPDATE, process_selinux);
		}
	}
	if (is_sub_gid && subgid_count != 0) {
		if (find_new_sub_gids(&sub_gid_start, &subgid_count) < 0) {
			fprintf (stderr,
			         _("%s: can't create subordinate group IDs\n"),
			         Prog);
			fail_exit(E_SUB_GID_UPDATE, process_selinux);
		}
	}
#endif				/* ENABLE_SUBIDS */

	usr_update (subuid_count, subgid_count, &flags);

	close_files (&flags);

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");
	sssd_flush_cache (SSSD_DB_PASSWD | SSSD_DB_GROUP);

	/*
	 * tallylog_reset needs to be able to lookup
	 * a valid existing user name,
	 * so we cannot call it before close_files()
	 */
	if (!lflg && getpwuid (user_id) != NULL) {
		tallylog_reset (user_name);
	}

#ifdef WITH_SELINUX
	if (Zflg) {
		if (set_seuser (user_name, user_selinux, user_selinux_range) != 0) {
			fprintf (stderr,
			         _("%s: warning: the user name %s to %s SELinux user mapping failed.\n"),
			         Prog, user_name, user_selinux);
#ifdef WITH_AUDIT
			audit_logger (AUDIT_ROLE_ASSIGN,
			              "add-selinux-user-mapping",
			              user_name, user_id, SHADOW_AUDIT_FAILURE);
#endif				/* WITH_AUDIT */
			fail_exit (E_SE_UPDATE, process_selinux);
		}
	}
#endif				/* WITH_SELINUX */

	if (mflg) {
		create_home (&flags);
		if (home_added) {
			copy_tree (def_template, prefix_user_home, false, true,
			           (uid_t)-1, user_id, (gid_t)-1, user_gid);
			copy_tree (def_usrtemplate, prefix_user_home, false, true,
			           (uid_t)-1, user_id, (gid_t)-1, user_gid);
		} else {
			fprintf (stderr,
			         _("%s: warning: the home directory %s already exists.\n"
			           "%s: Not copying any file from skel directory into it.\n"),
			         Prog, user_home, Prog);
		}

	}

	/* Do not create mail directory for system accounts */
	if (!rflg) {
		create_mail (&flags);
	}

	if (run_parts ("/etc/shadow-maint/useradd-post.d", user_name,
			"useradd")) {
		exit(1);
	}

	return E_SUCCESS;
}
