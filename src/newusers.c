/*
 * Copyright 1990 - 1993, Julianne Frances Haugh
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
 *
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
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
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
static char *Prog;
static int cflg = 0;
static int rflg = 0;	/* create a system account */
static int sflg = 0;

static char *crypt_method = NULL;
static long sha_rounds = 5000;

static int is_shadow;
#ifdef SHADOWGRP
static int is_shadow_grp;
static int gshadow_locked = 0;
#endif
static int passwd_locked = 0;
static int group_locked = 0;
static int shadow_locked = 0;

#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
#endif

/* local function prototypes */
static void usage (void);
static void fail_exit (int);
static int add_group (const char *, const char *, gid_t *, gid_t);
static int get_uid (const char *, uid_t *);
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
		char *endptr;
		long int i = strtoul (gid, &endptr, 10);
		if ((*endptr != '\0') && (errno != ERANGE)) {
			fprintf (stderr,
			         _("%s: group ID `%s' is not valid\n"),
			         Prog, gid);
			return -1;
		}
		if (   (getgrgid (i) != NULL)
		    || (gr_locate_gid (i) != NULL)) {
			/* The user will use this ID for her
			 * primary group */
			*ngid = i;
			return 0;
		}
		grent.gr_gid = i;
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
	}

	/* Check if this is a valid group name */
	if (check_group_name (grent.gr_name) == 0) {
		fprintf (stderr,
		         _("%s: invalid group name `%s'\n"),
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
			         _("%s: group %s is a shadow group, but does not exist in /etc/group\n"),
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
			fprintf (stderr,
			         _("%s: group %s created, failure during the creation of the corresponding gshadow group\n"),
			         Prog, grent.gr_name);
			return -1;
		}
	}
#endif

	return 0;
}

static int get_uid (const char *uid, uid_t *nuid) {
	const struct passwd *pwd = NULL;

	/*
	 * The first guess for the UID is either the numerical UID that the
	 * caller provided, or the next available UID.
	 */
	if (isdigit (uid[0])) {
		char *endptr;
		long int i = strtoul (uid, &endptr, 10);
		if ((*endptr != '\0') && (errno != ERANGE)) {
			fprintf (stderr,
			         _("%s: user ID `%s' is not valid\n"),
			         Prog, uid);
			return -1;
		}
		*nuid = i;
	} else {
		if ('\0' != uid[0]) {
			/* local, no need for xgetpwnam */
			pwd = getpwnam (uid);
			if (NULL == pwd) {
				pwd = pw_locate (uid);
			}

			if (NULL != pwd) {
				*nuid = pwd->pw_uid;
			} else {
				fprintf (stderr,
				         _("%s: user `%s' does not exist\n"),
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
	if (check_user_name (name) == 0) {
		fprintf (stderr,
		         _("%s: invalid user name `%s'\n"),
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

	return !pw_update (&pwent);
}

static void update_passwd (struct passwd *pwd, const char *password)
{
	void *crypt_arg = NULL;
	if (crypt_method != NULL) {
		if (sflg) {
			crypt_arg = &sha_rounds;
		}
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
		if (sflg) {
			crypt_arg = &sha_rounds;
		}
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
		if (   (crypt_method != NULL)
		    && (0 == strcmp(crypt_method, "NONE"))) {
			spent.sp_pwdp = (char *)password;
		} else {
			const char *salt = crypt_make_salt (crypt_method,
			                                    crypt_arg);
			spent.sp_pwdp = pw_encrypt (password, salt);
		}
		return !spw_update (&spent);
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
	spent.sp_lstchg = time ((time_t *) 0) / SCALE;
	spent.sp_min = getdef_num ("PASS_MIN_DAYS", 0);
	/* 10000 is infinity this week */
	spent.sp_max = getdef_num ("PASS_MAX_DAYS", 10000);
	spent.sp_warn = getdef_num ("PASS_WARN_AGE", -1);
	spent.sp_inact = -1;
	spent.sp_expire = -1;
	spent.sp_flag = -1;

	return !spw_update (&spent);
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
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv,
#ifdef USE_SHA_CRYPT
	                     "c:hs:",
#else
	                     "c:h",
#endif
	                     long_options, &option_index)) != -1) {
		switch (c) {
		case 'c':
			cflg = 1;
			crypt_method = optarg;
			break;
		case 'h':
			usage ();
			break;
#ifdef USE_SHA_CRYPT
		case 's':
			sflg = 1;
			if (!getlong(optarg, &sha_rounds)) {
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
		if (!freopen (argv[optind], "r", stdin)) {
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
	if (sflg && !cflg) {
		fprintf (stderr,
		         _("%s: %s flag is ONLY allowed with the %s flag\n"),
		         Prog, "-s", "-c");
		usage ();
	}

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
#ifdef USE_PAM
	int retval = PAM_SUCCESS;
	struct passwd *pampw;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("newusers", pampw->pw_name, &conv, &pamh);
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
	if (!pw_lock ()) {
		fprintf (stderr, _("%s: can't lock /etc/passwd.\n"), Prog);
		fail_exit (1);
	}
	passwd_locked++;
	if (is_shadow && !spw_lock ()) {
		fprintf (stderr, _("%s: can't lock /etc/shadow.\n"), Prog);
		fail_exit (1);
	}
	shadow_locked++;
	if (!gr_lock ()) {
		fprintf (stderr, _("%s: can't lock /etc/group.\n"), Prog);
		fail_exit (1);
	}
	group_locked++;
#ifdef SHADOWGRP
	if (is_shadow_grp && !sgr_lock ()) {
		fprintf (stderr, _("%s: can't lock /etc/gshadow.\n"), Prog);
		fail_exit (1);
	}
	gshadow_locked++;
#endif

	if (   (!pw_open (O_RDWR))
	    || (is_shadow && !spw_open (O_RDWR))
	    || !gr_open (O_RDWR)
#ifdef SHADOWGRP
	    || (is_shadow_grp && !sgr_open(O_RDWR))
#endif
	   ) {
		fprintf (stderr, _("%s: can't open files\n"), Prog);
		fail_exit (1);
	}
}

/*
 * close_files - close and unlock the password, group and shadow databases
 */
static void close_files (void)
{
	if (   (!pw_close ())
	    || (is_shadow && !spw_close ())
	    || !gr_close ()
#ifdef SHADOWGRP
	    || (is_shadow_grp && !sgr_close())
#endif
	   ) {
		fprintf (stderr, _("%s: error updating files\n"), Prog);
		fail_exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadow_grp) {
		(void) sgr_unlock();
		gshadow_locked--;
	}
#endif
	(void) gr_unlock ();
	group_locked--;
	if (is_shadow) {
		(void) spw_unlock ();
		shadow_locked--;
	}
	(void) pw_unlock ();
	passwd_locked--;
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

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

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
	while (fgets (buf, sizeof buf, stdin) != (char *) 0) {
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
				*cp++ = '\0';
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
		    && (get_uid (fields[2], &uid) != 0)) {
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
			         _("%s: line %d: cannot find user %s\n"),
			         Prog, line, fields[0]);
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
		if (fields[4][0]) {
			newpw.pw_gecos = fields[4];
		}

		if (fields[5][0]) {
			newpw.pw_dir = fields[5];
		}

		if (fields[6][0]) {
			newpw.pw_shell = fields[6];
		}

		if (newpw.pw_dir[0] && access (newpw.pw_dir, F_OK)) {
			if (mkdir (newpw.pw_dir,
			           0777 & ~getdef_num ("UMASK",
			                               GETDEF_DEFAULT_UMASK))) {
				fprintf (stderr,
				         _("%s: line %d: mkdir failed\n"), Prog,
				         line);
			} else if (chown
				   (newpw.pw_dir, newpw.pw_uid, newpw.pw_gid)) {
				fprintf (stderr,
				         _("%s: line %d: chown failed\n"), Prog,
				         line);
			}
		}

		/*
		 * Update the password entry with the new changes made.
		 */
		if (!pw_update (&newpw)) {
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
	if (errors) {
		fprintf (stderr,
		         _("%s: error detected, changes ignored\n"), Prog);
		(void) gr_unlock ();
		if (is_shadow) {
			spw_unlock ();
		}
		(void) pw_unlock ();
		fail_exit (1);
	}

	close_files ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	return 0;
}

