/*
 * Copyright (c) 1990 - 1993, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
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

/*
 *	newusers - create users from a batch file
 *
 *	newusers creates a collection of entries in /etc/passwd
 *	and related files by reading a passwd-format file and
 *	adding entries in the related directories.
 */

#include <config.h>

#ident "$Id$"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
#include "groupio.h"
#include "nscd.h"
#include "pwio.h"
#include "sgroupio.h"
#include "shadowio.h"
#include "chkname.h"
/*
 * Global variables
 */
char *Prog;

static bool cflg = false;
static bool rflg = false;	/* create a system account */
#ifdef USE_SHA_CRYPT
static bool sflg = false;
#endif

static char *crypt_method = NULL;
#ifdef USE_SHA_CRYPT
static long sha_rounds = 5000;
#endif

static bool is_shadow;
#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool pw_locked = false;
static bool gr_locked = false;
static bool spw_locked = false;

/* local function prototypes */
static void usage (void);
static void fail_exit (int);
static int add_group (const char *, const char *, gid_t *, gid_t);
static int get_user_id (const char *, uid_t *);
static int add_user (const char *, uid_t, gid_t);
static void update_passwd (struct passwd *, const char *);
static int add_passwd (struct passwd *, const char *);
static void process_flags (int argc, char **argv);
static void check_flags (void);
static void check_perms (void);
static void open_files (void);
static void close_files (void);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [options] [input]\n"
	                   "\n"
	                   "  -c, --crypt-method            the crypt method (one of %s)\n"
	                   "  -r, --system                  create system accounts\n"
	                   "%s"
	                   "\n"),
	                 Prog,
#ifndef USE_SHA_CRYPT
	                 "NONE DES MD5", ""
#else
	                 "NONE DES MD5 SHA256 SHA512",
	                 _("  -s, --sha-rounds              number of SHA rounds for the SHA*\n"
	                   "                                crypt algorithms\n")
#endif
	                 );
	exit (1);
}

/*
 * fail_exit - undo as much as possible
 */
static void fail_exit (int code)
{
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

	exit (code);
}

/*
 * add_group - create a new group or add a user to an existing group
 */
static int add_group (const char *name, const char *gid, gid_t *ngid, uid_t uid)
{
	const struct group *grp;
	struct group grent;
	char *members[1];
#ifdef SHADOWGRP
	const struct sgrp *sg;
#endif

	/*
	 * Start by seeing if the named group already exists. This will be
	 * very easy to deal with if it does.
	 */
	grp = getgrnam (gid);
	if (NULL == grp) {
		grp = gr_locate (gid);
	}
	if (NULL != grp) {
		/* The user will use this ID for her primary group */
		*ngid = grp->gr_gid;
		/* Don't check gshadow */
		return 0;
	}

	if (isdigit (gid[0])) {
		/*
		 * The GID is a number, which means either this is a brand
		 * new group, or an existing group.
		 */

		if (get_gid (gid, &grent.gr_gid) == 0) {
			fprintf (stderr,
			         _("%s: invalid group ID '%s'\n"),
			         Prog, gid);
			return -1;
		}

		/* Look in both the system database (getgrgid) and in the
		 * internal database (gr_locate_gid), which may contain
		 * uncommitted changes */
		if (   (getgrgid ((gid_t) grent.gr_gid) != NULL)
		    || (gr_locate_gid ((gid_t) grent.gr_gid) != NULL)) {
			/* The user will use this ID for her
			 * primary group */
			*ngid = (gid_t) grent.gr_gid;
			return 0;
		}

		/* Do not create groups with GID == (gid_t)-1 */
		if (grent.gr_gid == (gid_t)-1) {
			fprintf (stderr,
			         _("%s: invalid group ID '%s'\n"),
			         Prog, gid);
			return -1;
		}
	} else {
		/* The gid parameter can be "" or a name which is not
		 * already the name of an existing group.
		 * In both cases, figure out what group ID can be used.
		 */
		if (find_new_gid(rflg, &grent.gr_gid, &uid) < 0) {
			return -1;
		}
	}

	/*
	 * Now I have all of the fields required to create the new group.
	 */
	if (('\0' != gid[0]) && (!isdigit (gid[0]))) {
		grent.gr_name = xstrdup (gid);
	} else {
		grent.gr_name = xstrdup (name);
/* FIXME: check if the group exist */
	}

	/* Check if this is a valid group name */
	if (!is_valid_group_name (grent.gr_name)) {
		fprintf (stderr,
		         _("%s: invalid group name '%s'\n"),
		         Prog, grent.gr_name);
		free (grent.gr_name);
		return -1;
	}

	grent.gr_passwd = "x";	/* XXX warning: const */
	members[0] = NULL;
	grent.gr_mem = members;

	*ngid = grent.gr_gid;

#ifdef SHADOWGRP
	if (is_shadow_grp) {
		sg = sgr_locate (grent.gr_name);

		if (NULL != sg) {
			fprintf (stderr,
			         _("%s: group '%s' is a shadow group, but does not exist in /etc/group\n"),
			         Prog, grent.gr_name);
			return -1;
		}
	}
#endif

	if (gr_update (&grent) == 0) {
		return -1;
	}

#ifdef SHADOWGRP
	if (is_shadow_grp) {
		struct sgrp sgrent;
		char *admins[1];
		sgrent.sg_name = grent.gr_name;
		sgrent.sg_passwd = "*";	/* XXX warning: const */
		admins[0] = NULL;
		sgrent.sg_adm = admins;
		sgrent.sg_mem = members;

		if (sgr_update (&sgrent) == 0) {
			return -1;
		}
	}
#endif

	return 0;
}

static int get_user_id (const char *uid, uid_t *nuid) {

	/*
	 * The first guess for the UID is either the numerical UID that the
	 * caller provided, or the next available UID.
	 */
	if (isdigit (uid[0])) {
		if ((get_uid (uid, nuid) == 0) || (*nuid == (uid_t)-1)) {
			fprintf (stderr,
			         _("%s: invalid user ID '%s'\n"),
			         Prog, uid);
			return -1;
		}
	} else {
		if ('\0' != uid[0]) {
			const struct passwd *pwd;
			/* local, no need for xgetpwnam */
			pwd = getpwnam (uid);
			if (NULL == pwd) {
				pwd = pw_locate (uid);
			}

			if (NULL != pwd) {
				*nuid = pwd->pw_uid;
			} else {
				fprintf (stderr,
				         _("%s: user '%s' does not exist\n"),
				         Prog, uid);
				return -1;
			}
		} else {
			if (find_new_uid (rflg, nuid, NULL) < 0) {
				return -1;
			}
		}
	}

	return 0;
}

/*
 * add_user - create a new user ID
 */
static int add_user (const char *name, uid_t uid, gid_t gid)
{
	struct passwd pwent;

	/* Check if this is a valid user name */
	if (!is_valid_user_name (name)) {
		fprintf (stderr,
		         _("%s: invalid user name '%s'\n"),
		         Prog, name);
		return -1;
	}

	/*
	 * I don't want to fill in the entire password structure members
	 * JUST YET, since there is still more data to be added. So, I fill
	 * in the parts that I have.
	 */
	pwent.pw_name = xstrdup (name);
	pwent.pw_uid = uid;
	pwent.pw_passwd = "x";	/* XXX warning: const */
	pwent.pw_gid = gid;
	pwent.pw_gecos = "";	/* XXX warning: const */
	pwent.pw_dir = "";	/* XXX warning: const */
	pwent.pw_shell = "";	/* XXX warning: const */

	return (pw_update (&pwent) == 0);
}

static void update_passwd (struct passwd *pwd, const char *password)
{
	void *crypt_arg = NULL;
	if (crypt_method != NULL) {
#ifdef USE_SHA_CRYPT
		if (sflg) {
			crypt_arg = &sha_rounds;
		}
#endif
	}

	if ((crypt_method != NULL) && (0 == strcmp(crypt_method, "NONE"))) {
		pwd->pw_passwd = (char *)password;
	} else {
		pwd->pw_passwd = pw_encrypt (password,
		                             crypt_make_salt (crypt_method,
		                                              crypt_arg));
	}
}

/*
 * add_passwd - add or update the encrypted password
 */
static int add_passwd (struct passwd *pwd, const char *password)
{
	const struct spwd *sp;
	struct spwd spent;
	void *crypt_arg = NULL;
	if (crypt_method != NULL) {
#ifdef USE_SHA_CRYPT
		if (sflg) {
			crypt_arg = &sha_rounds;
		}
#endif
	}

	/*
	 * In the case of regular password files, this is real easy - pwd
	 * points to the entry in the password file. Shadow files are
	 * harder since there are zillions of things to do ...
	 */
	if (!is_shadow) {
		update_passwd (pwd, password);
		return 0;
	}

	/*
	 * Do the first and easiest shadow file case. The user already
	 * exists in the shadow password file.
	 */
	sp = spw_locate (pwd->pw_name);
	if (NULL != sp) {
		spent = *sp;
		if (   (NULL != crypt_method)
		    && (0 == strcmp(crypt_method, "NONE"))) {
			spent.sp_pwdp = (char *)password;
		} else {
			const char *salt = crypt_make_salt (crypt_method,
			                                    crypt_arg);
			spent.sp_pwdp = pw_encrypt (password, salt);
		}
		return (spw_update (&spent) == 0);
	}

	/*
	 * Pick the next easiest case - the user has an encrypted password
	 * which isn't equal to "x". The password was set to "x" earlier
	 * when the entry was created, so this user would have to have had
	 * the password set someplace else.
	 */
	if (strcmp (pwd->pw_passwd, "x") != 0) {
		update_passwd (pwd, password);
		return 0;
	}

	/*
	 * Now the really hard case - I need to create an entirely new
	 * shadow password file entry.
	 */
	spent.sp_namp = pwd->pw_name;
	if ((crypt_method != NULL) && (0 == strcmp(crypt_method, "NONE"))) {
		spent.sp_pwdp = (char *)password;
	} else {
		const char *salt = crypt_make_salt (crypt_method, crypt_arg);
		spent.sp_pwdp = pw_encrypt (password, salt);
	}
	spent.sp_lstchg = (long) time ((time_t *) 0) / SCALE;
	spent.sp_min    = getdef_num ("PASS_MIN_DAYS", 0);
	/* 10000 is infinity this week */
	spent.sp_max    = getdef_num ("PASS_MAX_DAYS", 10000);
	spent.sp_warn   = getdef_num ("PASS_WARN_AGE", -1);
	spent.sp_inact  = -1;
	spent.sp_expire = -1;
	spent.sp_flag   = SHADOW_SP_FLAG_UNSET;

	return (spw_update (&spent) == 0);
}

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
{
	int option_index = 0;
	int c;
	static struct option long_options[] = {
		{"crypt-method", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
#ifdef USE_SHA_CRYPT
		{"sha-rounds", required_argument, NULL, 's'},
#endif
		{"system", no_argument, NULL, 'r'},
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv,
#ifdef USE_SHA_CRYPT
	                     "c:hrs:",
#else
	                     "c:hr",
#endif
	                     long_options, &option_index)) != -1) {
		switch (c) {
		case 'c':
			cflg = true;
			crypt_method = optarg;
			break;
		case 'h':
			usage ();
			break;
		case 'r':
			rflg = true;
			break;
#ifdef USE_SHA_CRYPT
		case 's':
			sflg = true;
			if (getlong(optarg, &sha_rounds) == 0) {
				fprintf (stderr,
				         _("%s: invalid numeric argument '%s'\n"),
				         Prog, optarg);
				usage ();
			}
			break;
#endif
		case 0:
			/* long option */
			break;
		default:
			usage ();
			break;
		}
	}

	if (argv[optind] != NULL) {
		if (freopen (argv[optind], "r", stdin) == NULL) {
			char buf[BUFSIZ];
			snprintf (buf, sizeof buf, "%s: %s", Prog, argv[1]);
			perror (buf);
			fail_exit (1);
		}
	}

	/* validate options */
	check_flags ();
}

/*
 * check_flags - check flags and parameters consistency
 *
 *	It will not return if an error is encountered.
 */
static void check_flags (void)
{
#ifdef USE_SHA_CRYPT
	if (sflg && !cflg) {
		fprintf (stderr,
		         _("%s: %s flag is only allowed with the %s flag\n"),
		         Prog, "-s", "-c");
		usage ();
	}
#endif

	if (cflg) {
		if (   (0 != strcmp (crypt_method, "DES"))
		    && (0 != strcmp (crypt_method, "MD5"))
		    && (0 != strcmp (crypt_method, "NONE"))
#ifdef USE_SHA_CRYPT
		    && (0 != strcmp (crypt_method, "SHA256"))
		    && (0 != strcmp (crypt_method, "SHA512"))
#endif
		    ) {
			fprintf (stderr,
			         _("%s: unsupported crypt method: %s\n"),
			         Prog, crypt_method);
			usage ();
		}
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	With PAM support, the setuid bit can be set on groupadd to allow
 *	non-root users to groups.
 *	Without PAM support, only users who can write in the group databases
 *	can add groups.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms (void)
{
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retval;
	struct passwd *pampw;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (NULL == pampw) {
		fprintf (stderr,
		         _("%s: Cannot determine your user name.\n"),
		         Prog);
		fail_exit (1);
	}

	retval = pam_start ("newusers", pampw->pw_name, &conv, &pamh);

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
}

/*
 * open_files - lock and open the password, group and shadow databases
 */
static void open_files (void)
{
	/*
	 * Lock the password files and open them for update. This will bring
	 * all of the entries into memory where they may be searched for an
	 * modified, or new entries added. The password file is the key - if
	 * it gets locked, assume the others can be locked right away.
	 */
	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
		fail_exit (1);
	}
	pw_locked = true;
	if (is_shadow) {
		if (spw_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, spw_dbname ());
			fail_exit (1);
		}
		spw_locked = true;
	}
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
		fail_exit (1);
	}
	gr_locked = true;
#ifdef SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			fail_exit (1);
		}
		sgr_locked = true;
	}
#endif

	if (pw_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, pw_dbname ());
		fail_exit (1);
	}
	if (is_shadow && (spw_open (O_RDWR) == 0)) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, spw_dbname ());
		fail_exit (1);
	}
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, gr_dbname ());
		fail_exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadow_grp && (sgr_open (O_RDWR) == 0)) {
		fprintf (stderr, _("%s: cannot open %s\n"), Prog, sgr_dbname ());
		fail_exit (1);
	}
#endif
}

/*
 * close_files - close and unlock the password, group and shadow databases
 */
static void close_files (void)
{
	if (pw_close () == 0) {
		fprintf (stderr, _("%s: failure while writing changes to %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", pw_dbname ()));
		fail_exit (1);
	}
	if (pw_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
		/* continue */
	}
	pw_locked = false;

	if (is_shadow) {
		if (spw_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", spw_dbname ()));
			fail_exit (1);
		}
		if (spw_unlock () == 0) {
			fprintf (stderr,
			         _("%s: failed to unlock %s\n"),
			         Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
		spw_locked = false;
	}

	if (gr_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", gr_dbname ()));
		fail_exit (1);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr,
		         _("%s: failed to unlock %s\n"),
		         Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
		/* continue */
	}
	gr_locked = false;

#ifdef SHADOWGRP
	if (is_shadow_grp) {
		if (sgr_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", sgr_dbname ()));
			fail_exit (1);
		}
		if (sgr_unlock () == 0) {
			fprintf (stderr,
			         _("%s: failed to unlock %s\n"),
			         Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
			/* continue */
		}
		sgr_locked = false;
	}
#endif
}

int main (int argc, char **argv)
{
	char buf[BUFSIZ];
	char *fields[8];
	int nfields;
	char *cp;
	const struct passwd *pw;
	struct passwd newpw;
	int errors = 0;
	int line = 0;
	uid_t uid;
	gid_t gid;

	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	OPENLOG ("newusers");

	process_flags (argc, argv);

	check_perms ();

	is_shadow = spw_file_present ();

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	open_files ();

	/*
	 * Read each line. The line has the same format as a password file
	 * entry, except that certain fields are not constrained to be
	 * numerical values. If a group ID is entered which does not already
	 * exist, an attempt is made to allocate the same group ID as the
	 * numerical user ID. Should that fail, the next available group ID
	 * over 100 is allocated. The pw_gid field will be updated with that
	 * value.
	 */
	while (fgets (buf, (int) sizeof buf, stdin) != (char *) 0) {
		line++;
		cp = strrchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		} else {
			fprintf (stderr, _("%s: line %d: line too long\n"),
			         Prog, line);
			errors++;
			continue;
		}

		/*
		 * Break the string into fields and screw around with them.
		 * There MUST be 7 colon separated fields, although the
		 * values aren't that particular.
		 */
		for (cp = buf, nfields = 0; nfields < 7; nfields++) {
			fields[nfields] = cp;
			cp = strchr (cp, ':');
			if (NULL != cp) {
				*cp = '\0';
				cp++;
			} else {
				break;
			}
		}
		if (nfields != 6) {
			fprintf (stderr, _("%s: line %d: invalid line\n"),
			         Prog, line);
			continue;
		}

		/*
		 * First check if we have to create of update an user
		 */
		pw = pw_locate (fields[0]);
		/* local, no need for xgetpwnam */
		if (   (NULL == pw)
		    && (getpwnam (fields[0]) != NULL)) {
			fprintf (stderr, _("%s: cannot update the entry of user %s (not in the passwd database)\n"), Prog, fields[0]);
			errors++;
			continue;
		}

		if (   (NULL == pw)
		    && (get_user_id (fields[2], &uid) != 0)) {
			fprintf (stderr,
			         _("%s: line %d: can't create user\n"),
			         Prog, line);
			errors++;
			continue;
		}

		/*
		 * Processed is the group name. A new group will be
		 * created if the group name is non-numeric and does not
		 * already exist. If the group name is a number (which is not
		 * an existing GID), a group with the same name as the user
		 * will be created, with the given GID. The given or created
		 * group will be the primary group of the user. If
		 * there is no named group to be a member of, the UID will
		 * be figured out and that value will be a candidate for a
		 * new group, if that group ID exists, a whole new group ID
		 * will be made up.
		 */
		if (   (NULL == pw)
		    && (add_group (fields[0], fields[3], &gid, uid) != 0)) {
			fprintf (stderr,
			         _("%s: line %d: can't create group\n"),
			         Prog, line);
			errors++;
			continue;
		}

		/*
		 * Now we work on the user ID. It has to be specified either
		 * as a numerical value, or left blank. If it is a numerical
		 * value, that value will be used, otherwise the next
		 * available user ID is computed and used. After this there
		 * will at least be a (struct passwd) for the user.
		 */
		if (   (NULL == pw)
		    && (add_user (fields[0], uid, gid) != 0)) {
			fprintf (stderr,
			         _("%s: line %d: can't create user\n"),
			         Prog, line);
			errors++;
			continue;
		}

		/*
		 * The password, gecos field, directory, and shell fields
		 * all come next.
		 */
		pw = pw_locate (fields[0]);
		if (NULL == pw) {
			fprintf (stderr,
			         _("%s: line %d: user '%s' does not exist in %s\n"),
			         Prog, line, fields[0], pw_dbname ());
			errors++;
			continue;
		}
		newpw = *pw;

		if (add_passwd (&newpw, fields[1])) {
			fprintf (stderr,
			         _("%s: line %d: can't update password\n"),
			         Prog, line);
			errors++;
			continue;
		}
		if ('\0' != fields[4][0]) {
			newpw.pw_gecos = fields[4];
		}

		if ('\0' != fields[5][0]) {
			newpw.pw_dir = fields[5];
		}

		if ('\0' != fields[6][0]) {
			newpw.pw_shell = fields[6];
		}

		if (   ('\0' != newpw.pw_dir[0])
		    && (access (newpw.pw_dir, F_OK) != 0)) {
/* FIXME: should check for directory */
			mode_t msk = 0777 & ~getdef_num ("UMASK",
			                                 GETDEF_DEFAULT_UMASK);
			if (mkdir (newpw.pw_dir, msk) != 0) {
				fprintf (stderr,
				         _("%s: line %d: mkdir failed\n"), Prog,
				         line);
			} else if (chown (newpw.pw_dir,
			                  newpw.pw_uid,
			                  newpw.pw_gid) != 0) {
				fprintf (stderr,
				         _("%s: line %d: chown failed\n"), Prog,
				         line);
			}
		}

		/*
		 * Update the password entry with the new changes made.
		 */
		if (pw_update (&newpw) == 0) {
			fprintf (stderr,
			         _("%s: line %d: can't update entry\n"),
			         Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes to be
	 * aborted. Unlocking the password file will cause all of the
	 * changes to be ignored. Otherwise the file is closed, causing the
	 * changes to be written out all at once, and then unlocked
	 * afterwards.
	 */
	if (0 != errors) {
		fprintf (stderr,
		         _("%s: error detected, changes ignored\n"), Prog);
		fail_exit (1);
	}

	close_files ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

	return 0;
}

