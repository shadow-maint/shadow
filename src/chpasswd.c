/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
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

#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include "defines.h"
#include "nscd.h"
#include "getdef.h"
#include "prototypes.h"
#include "pwio.h"
#include "shadowio.h"
/*@-exitarg@*/
#include "exitcodes.h"

/*
 * Global variables
 */
const char *Prog;
static bool eflg   = false;
static bool md5flg = false;
#ifdef USE_SHA_CRYPT
static bool sflg   = false;
#endif				/* USE_SHA_CRYPT */

static /*@null@*//*@observer@*/const char *crypt_method = NULL;
#define cflg (NULL != crypt_method)
#ifdef USE_SHA_CRYPT
static long sha_rounds = 5000;
#endif				/* USE_SHA_CRYPT */

static bool is_shadow_pwd;
static bool pw_locked = false;
static bool spw_locked = false;

/* local function prototypes */
static void fail_exit (int code);
static /*@noreturn@*/void usage (int status);
static void process_flags (int argc, char **argv);
static void check_flags (void);
static void check_perms (void);
static void open_files (void);
static void close_files (void);

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

	if (spw_locked) {
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
	}

	exit (code);
}

/*
 * usage - display usage message and exit
 */
static /*@noreturn@*/void usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options]\n"
	                  "\n"
	                  "Options:\n"),
	                Prog);
	(void) fprintf (usageout,
	                _("  -c, --crypt-method METHOD     the crypt method (one of %s)\n"),
#ifndef USE_SHA_CRYPT
	                "NONE DES MD5"
#else				/* USE_SHA_CRYPT */
	                "NONE DES MD5 SHA256 SHA512"
#endif				/* USE_SHA_CRYPT */
	               );
	(void) fputs (_("  -e, --encrypted               supplied passwords are encrypted\n"), usageout);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -m, --md5                     encrypt the clear text password using\n"
	                "                                the MD5 algorithm\n"),
	              usageout);
	(void) fputs (_("  -R, --root CHROOT_DIR         directory to chroot into\n"), usageout);
#ifdef USE_SHA_CRYPT
	(void) fputs (_("  -s, --sha-rounds              number of SHA rounds for the SHA*\n"
	                "                                crypt algorithms\n"),
	              usageout);
#endif				/* USE_SHA_CRYPT */
	(void) fputs ("\n", usageout);

	exit (status);
}

/*
 * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
{
	int c;
	static struct option long_options[] = {
		{"crypt-method", required_argument, NULL, 'c'},
		{"encrypted",    no_argument,       NULL, 'e'},
		{"help",         no_argument,       NULL, 'h'},
		{"md5",          no_argument,       NULL, 'm'},
		{"root",         required_argument, NULL, 'R'},
#ifdef USE_SHA_CRYPT
		{"sha-rounds",   required_argument, NULL, 's'},
#endif				/* USE_SHA_CRYPT */
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv,
#ifdef USE_SHA_CRYPT
	                         "c:ehmR:s:",
#else				/* !USE_SHA_CRYPT */
	                         "c:ehmR:",
#endif				/* !USE_SHA_CRYPT */
	                         long_options, NULL)) != -1) {
		switch (c) {
		case 'c':
			crypt_method = optarg;
			break;
		case 'e':
			eflg = true;
			break;
		case 'h':
			usage (E_SUCCESS);
			/*@notreached@*/break;
		case 'm':
			md5flg = true;
			break;
		case 'R': /* no-op, handled in process_root_flag () */
			break;
#ifdef USE_SHA_CRYPT
		case 's':
			sflg = true;
			if (getlong(optarg, &sha_rounds) == 0) {
				fprintf (stderr,
				         _("%s: invalid numeric argument '%s'\n"),
				         Prog, optarg);
				usage (E_USAGE);
			}
			break;
#endif				/* USE_SHA_CRYPT */
		default:
			usage (E_USAGE);
			/*@notreached@*/break;
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
		usage (E_USAGE);
	}
#endif

	if ((eflg && (md5flg || cflg)) ||
	    (md5flg && cflg)) {
		fprintf (stderr,
		         _("%s: the -c, -e, and -m flags are exclusive\n"),
		         Prog);
		usage (E_USAGE);
	}

	if (cflg) {
		if (   (0 != strcmp (crypt_method, "DES"))
		    && (0 != strcmp (crypt_method, "MD5"))
		    && (0 != strcmp (crypt_method, "NONE"))
#ifdef USE_SHA_CRYPT
		    && (0 != strcmp (crypt_method, "SHA256"))
		    && (0 != strcmp (crypt_method, "SHA512"))
#endif				/* USE_SHA_CRYPT */
		    ) {
			fprintf (stderr,
			         _("%s: unsupported crypt method: %s\n"),
			         Prog, crypt_method);
			usage (E_USAGE);
		}
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	With PAM support, the setuid bit can be set on chpasswd to allow
 *	non-root users to groups.
 *	Without PAM support, only users who can write in the group databases
 *	can add groups.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms (void)
{
#ifdef USE_PAM
#ifdef ACCT_TOOLS_SETUID
	/* If chpasswd uses PAM and is SUID, check the permissions,
	 * otherwise, the permissions are enforced by the access to the
	 * passwd and shadow files.
	 */
	pam_handle_t *pamh = NULL;
	int retval;
	struct passwd *pampw;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (NULL == pampw) {
		fprintf (stderr,
		         _("%s: Cannot determine your user name.\n"),
		         Prog);
		exit (1);
	}

	retval = pam_start ("chpasswd", pampw->pw_name, &conv, &pamh);

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
		exit (1);
	}
	(void) pam_end (pamh, retval);
#endif				/* ACCT_TOOLS_SETUID */
#endif				/* USE_PAM */
}

/*
 * open_files - lock and open the password databases
 */
static void open_files (void)
{
	/*
	 * Lock the password file and open it for reading and writing. This
	 * will bring all of the entries into memory where they may be updated.
	 */
	if (pw_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, pw_dbname ());
		fail_exit (1);
	}
	pw_locked = true;
	if (pw_open (O_CREAT | O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, pw_dbname ());
		fail_exit (1);
	}

	/* Do the same for the shadowed database, if it exist */
	if (is_shadow_pwd) {
		if (spw_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, spw_dbname ());
			fail_exit (1);
		}
		spw_locked = true;
		if (spw_open (O_CREAT | O_RDWR) == 0) {
			fprintf (stderr,
			         _("%s: cannot open %s\n"),
			         Prog, spw_dbname ());
			fail_exit (1);
		}
	}
}

/*
 * close_files - close and unlock the password databases
 */
static void close_files (void)
{
	if (is_shadow_pwd) {
		if (spw_close () == 0) {
			fprintf (stderr,
			         _("%s: failure while writing changes to %s\n"),
			         Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failure while writing changes to %s", spw_dbname ()));
			fail_exit (1);
		}
		if (spw_unlock () == 0) {
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, spw_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", spw_dbname ()));
			/* continue */
		}
		spw_locked = false;
	}

	if (pw_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", pw_dbname ()));
		fail_exit (1);
	}
	if (pw_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, pw_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", pw_dbname ()));
		/* continue */
	}
	pw_locked = false;
}

int main (int argc, char **argv)
{
	char buf[BUFSIZ];
	char *name;
	char *newpwd;
	char *cp;

#ifdef USE_PAM
	bool use_pam = true;
#endif				/* USE_PAM */

	int errors = 0;
	int line = 0;

	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_root_flag ("-R", argc, argv);

	process_flags (argc, argv);

#ifdef USE_PAM
	if (md5flg || eflg || cflg) {
		use_pam = false;
	}
#endif				/* USE_PAM */

	OPENLOG ("chpasswd");

	check_perms ();

#ifdef USE_PAM
	if (!use_pam)
#endif				/* USE_PAM */
	{
		is_shadow_pwd = spw_file_present ();

		open_files ();
	}

	/*
	 * Read each line, separating the user name from the password. The
	 * password entry for each user will be looked up in the appropriate
	 * file (shadow or passwd) and the password changed. For shadow
	 * files the last change date is set directly, for passwd files the
	 * last change date is set in the age only if aging information is
	 * present.
	 */
	while (fgets (buf, (int) sizeof buf, stdin) != (char *) 0) {
		line++;
		cp = strrchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		} else {
			if (feof (stdin) == 0) {
				fprintf (stderr,
				         _("%s: line %d: line too long\n"),
				         Prog, line);
				errors++;
				continue;
			}
		}

		/*
		 * The username is the first field. It is separated from the
		 * password with a ":" character which is replaced with a
		 * NUL to give the new password. The new password will then
		 * be encrypted in the normal fashion with a new salt
		 * generated, unless the '-e' is given, in which case it is
		 * assumed to already be encrypted.
		 */

		name = buf;
		cp = strchr (name, ':');
		if (NULL != cp) {
			*cp = '\0';
			cp++;
		} else {
			fprintf (stderr,
			         _("%s: line %d: missing new password\n"),
			         Prog, line);
			errors++;
			continue;
		}
		newpwd = cp;

#ifdef USE_PAM
		if (use_pam){
			if (do_pam_passwd_non_interactive ("chpasswd", name, newpwd) != 0) {
				fprintf (stderr,
				         _("%s: (line %d, user %s) password not changed\n"),
				         Prog, line, name);
				errors++;
			}
		} else
#endif				/* USE_PAM */
		{
		const struct spwd *sp;
		struct spwd newsp;
		const struct passwd *pw;
		struct passwd newpw;

		if (   !eflg
		    && (   (NULL == crypt_method)
		        || (0 != strcmp (crypt_method, "NONE")))) {
			void *arg = NULL;
			const char *salt;
			if (md5flg) {
				crypt_method = "MD5";
			}
#ifdef USE_SHA_CRYPT
			if (sflg) {
				arg = &sha_rounds;
			}
#endif
			salt = crypt_make_salt (crypt_method, arg);
			cp = pw_encrypt (newpwd, salt);
			if (NULL == cp) {
				fprintf (stderr,
				         _("%s: failed to crypt password with salt '%s': %s\n"),
				         Prog, salt, strerror (errno));
				fail_exit (1);
			}
		}

		/*
		 * Get the password file entry for this user. The user must
		 * already exist.
		 */
		pw = pw_locate (name);
		if (NULL == pw) {
			fprintf (stderr,
			         _("%s: line %d: user '%s' does not exist\n"), Prog,
			         line, name);
			errors++;
			continue;
		}
		if (is_shadow_pwd) {
			/* The shadow entry should be updated if the
			 * passwd entry has a password set to 'x'.
			 * But on the other hand, if there is already both
			 * a passwd and a shadow password, it's preferable
			 * to update both.
			 */
			sp = spw_locate (name);

			if (   (NULL == sp)
			    && (strcmp (pw->pw_passwd,
			                SHADOW_PASSWD_STRING) == 0)) {
				/* If the password is set to 'x' in
				 * passwd, but there are no entries in
				 * shadow, create one.
				 */
				newsp.sp_namp  = name;
				/* newsp.sp_pwdp  = NULL; will be set later */
				/* newsp.sp_lstchg= 0;    will be set later */
				newsp.sp_min   = getdef_num ("PASS_MIN_DAYS", -1);
				newsp.sp_max   = getdef_num ("PASS_MAX_DAYS", -1);
				newsp.sp_warn  = getdef_num ("PASS_WARN_AGE", -1);
				newsp.sp_inact = -1;
				newsp.sp_expire= -1;
				newsp.sp_flag  = SHADOW_SP_FLAG_UNSET;
				sp = &newsp;
			}
		} else {
			sp = NULL;
		}

		/*
		 * The freshly encrypted new password is merged into the
		 * user's password file entry and the last password change
		 * date is set to the current date.
		 */
		if (NULL != sp) {
			newsp = *sp;
			newsp.sp_pwdp = cp;
			newsp.sp_lstchg = (long) gettime () / SCALE;
			if (0 == newsp.sp_lstchg) {
				/* Better disable aging than requiring a
				 * password change */
				newsp.sp_lstchg = -1;
			}
		}

		if (   (NULL == sp)
		    || (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) != 0)) {
			newpw = *pw;
			newpw.pw_passwd = cp;
		}

		/* 
		 * The updated password file entry is then put back and will
		 * be written to the password file later, after all the
		 * other entries have been updated as well.
		 */
		if (NULL != sp) {
			if (spw_update (&newsp) == 0) {
				fprintf (stderr,
				         _("%s: line %d: failed to prepare the new %s entry '%s'\n"),
				         Prog, line, spw_dbname (), newsp.sp_namp);
				errors++;
				continue;
			}
		}
		if (   (NULL == sp)
		    || (strcmp (pw->pw_passwd, SHADOW_PASSWD_STRING) != 0)) {
			if (pw_update (&newpw) == 0) {
				fprintf (stderr,
				         _("%s: line %d: failed to prepare the new %s entry '%s'\n"),
				         Prog, line, pw_dbname (), newpw.pw_name);
				errors++;
				continue;
			}
		}
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes to be
	 * aborted. Unlocking the password file will cause all of the
	 * changes to be ignored. Otherwise the file is closed, causing the
	 * changes to be written out all at once, and then unlocked
	 * afterwards.
	 *
	 * With PAM, it is not possible to delay the update of the
	 * password database.
	 */
	if (0 != errors) {
#ifdef USE_PAM
		if (!use_pam)
#endif				/* USE_PAM */
		{
			fprintf (stderr,
			         _("%s: error detected, changes ignored\n"),
			         Prog);
		}
		fail_exit (1);
	}

#ifdef USE_PAM
	if (!use_pam)
#endif				/* USE_PAM */
	{
	/* Save the changes */
		close_files ();
	}

	nscd_flush_cache ("passwd");

	return (0);
}

