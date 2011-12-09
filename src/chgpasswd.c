/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 2006       , Tomasz Kłoczko
 * Copyright (c) 2006       , Jonas Meurer
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
#ifdef ACCT_TOOLS_SETUID
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
#include "defines.h"
#include "nscd.h"
#include "prototypes.h"
#include "groupio.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
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
#endif

static /*@null@*//*@observer@*/const char *crypt_method = NULL;
#define cflg (NULL != crypt_method)
#ifdef USE_SHA_CRYPT
static long sha_rounds = 5000;
#endif

#ifdef SHADOWGRP
static bool is_shadow_grp;
static bool sgr_locked = false;
#endif
static bool gr_locked = false;

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
#endif
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv,
#ifdef USE_SHA_CRYPT
	                         "c:ehmR:s:",
#else
	                         "c:ehmR:",
#endif
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
#endif
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
#endif
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
 *	With PAM support, the setuid bit can be set on chgpasswd to allow
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
		exit (1);
	}

	retval = pam_start ("chgpasswd", pampw->pw_name, &conv, &pamh);

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
#endif				/* USE_PAM */
#endif				/* ACCT_TOOLS_SETUID */
}

/*
 * open_files - lock and open the group databases
 */
static void open_files (void)
{
	/*
	 * Lock the group file and open it for reading and writing. This will
	 * bring all of the entries into memory where they may be updated.
	 */
	if (gr_lock () == 0) {
		fprintf (stderr,
		         _("%s: cannot lock %s; try again later.\n"),
		         Prog, gr_dbname ());
		fail_exit (1);
	}
	gr_locked = true;
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: cannot open %s\n"), Prog, gr_dbname ());
		fail_exit (1);
	}

#ifdef SHADOWGRP
	/* Do the same for the shadowed database, if it exist */
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr,
			         _("%s: cannot lock %s; try again later.\n"),
			         Prog, sgr_dbname ());
			fail_exit (1);
		}
		sgr_locked = true;
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr, _("%s: cannot open %s\n"),
			         Prog, sgr_dbname ());
			fail_exit (1);
		}
	}
#endif
}

/*
 * close_files - close and unlock the group databases
 */
static void close_files (void)
{
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
			fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, sgr_dbname ());
			SYSLOG ((LOG_ERR, "failed to unlock %s", sgr_dbname ()));
			/* continue */
		}
		sgr_locked = false;
	}
#endif

	if (gr_close () == 0) {
		fprintf (stderr,
		         _("%s: failure while writing changes to %s\n"),
		         Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failure while writing changes to %s", gr_dbname ()));
		fail_exit (1);
	}
	if (gr_unlock () == 0) {
		fprintf (stderr, _("%s: failed to unlock %s\n"), Prog, gr_dbname ());
		SYSLOG ((LOG_ERR, "failed to unlock %s", gr_dbname ()));
		/* continue */
	}
	gr_locked = false;
}

int main (int argc, char **argv)
{
	char buf[BUFSIZ];
	char *name;
	char *newpwd;
	char *cp;

#ifdef	SHADOWGRP
	const struct sgrp *sg;
	struct sgrp newsg;
#endif

	const struct group *gr;
	struct group newgr;
	int errors = 0;
	int line = 0;

	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_root_flag ("-R", argc, argv);

	process_flags (argc, argv);

	OPENLOG ("chgpasswd");

	check_perms ();

#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
#endif

	open_files ();

	/*
	 * Read each line, separating the group name from the password. The
	 * group entry for each group will be looked up in the appropriate
	 * file (gshadow or group) and the password changed.
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
		 * The group's name is the first field. It is separated from
		 * the password with a ":" character which is replaced with a
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
		if (   (!eflg)
		    && (   (NULL == crypt_method)
		        || (0 != strcmp (crypt_method, "NONE")))) {
			void *arg = NULL;
			if (md5flg) {
				crypt_method = "MD5";
			}
#ifdef USE_SHA_CRYPT
			if (sflg) {
				arg = &sha_rounds;
			}
#endif
			cp = pw_encrypt (newpwd,
			                 crypt_make_salt (crypt_method, arg));
		}

		/*
		 * Get the group file entry for this group. The group must
		 * already exist.
		 */
		gr = gr_locate (name);
		if (NULL == gr) {
			fprintf (stderr,
			         _("%s: line %d: group '%s' does not exist\n"), Prog,
			         line, name);
			errors++;
			continue;
		}
#ifdef SHADOWGRP
		if (is_shadow_grp) {
			/* The gshadow entry should be updated if the
			 * group entry has a password set to 'x'.
			 * But on the other hand, if there is already both
			 * a group and a gshadow password, it's preferable
			 * to update both.
			 */
			sg = sgr_locate (name);

			if (   (NULL == sg)
			    && (strcmp (gr->gr_passwd,
			                SHADOW_PASSWD_STRING) == 0)) {
				static char *empty = NULL;
				/* If the password is set to 'x' in
				 * group, but there are no entries in
				 * gshadow, create one.
				 */
				newsg.sg_name   = name;
				/* newsg.sg_passwd = NULL; will be set later */
				newsg.sg_adm    = &empty;
				newsg.sg_mem    = dup_list (gr->gr_mem);
				sg = &newsg;
			}
		} else {
			sg = NULL;
		}
#endif

		/*
		 * The freshly encrypted new password is merged into the
		 * group's entry.
		 */
#ifdef SHADOWGRP
		if (NULL != sg) {
			newsg = *sg;
			newsg.sg_passwd = cp;
		}
		if (   (NULL == sg)
		    || (strcmp (gr->gr_passwd, SHADOW_PASSWD_STRING) != 0))
#endif
		{
			newgr = *gr;
			newgr.gr_passwd = cp;
		}

		/* 
		 * The updated group file entry is then put back and will
		 * be written to the group file later, after all the
		 * other entries have been updated as well.
		 */
#ifdef SHADOWGRP
		if (NULL != sg) {
			if (sgr_update (&newsg) == 0) {
				fprintf (stderr,
				         _("%s: line %d: failed to prepare the new %s entry '%s'\n"),
				         Prog, line, sgr_dbname (), newsg.sg_name);
				errors++;
				continue;
			}
		}
		if (   (NULL == sg)
		    || (strcmp (gr->gr_passwd, SHADOW_PASSWD_STRING) != 0))
#endif
		{
			if (gr_update (&newgr) == 0) {
				fprintf (stderr,
				         _("%s: line %d: failed to prepare the new %s entry '%s'\n"),
				         Prog, line, gr_dbname (), newgr.gr_name);
				errors++;
				continue;
			}
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes to be
	 * aborted. Unlocking the group file will cause all of the
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

	nscd_flush_cache ("group");

	return (0);
}

