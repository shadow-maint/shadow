/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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
#include "prototypes.h"
#include "groupio.h"
#ifdef	SHADOWGRP
#include "sgroupio.h"
#endif
/*
 * Global variables
 */
static char *Prog;
static int cflg = 0;
static int eflg = 0;
static int md5flg = 0;
static int sflg = 0;

static char *crypt_method = NULL;
static long sha_rounds = 5000;

#ifdef SHADOWGRP
static int is_shadow_grp;
#endif

/* local function prototypes */
static void usage (void);
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
	fprintf (stderr, _("Usage: %s [options]\n"
	                   "\n"
	                   "Options:\n"
	                   "  -c, --crypt-method            the crypt method (one of %s)\n"
	                   "  -e, --encrypted               supplied passwords are encrypted\n"
	                   "  -h, --help                    display this help message and exit\n"
	                   "  -m, --md5                     encrypt the clear text password using\n"
	                   "                                the MD5 algorithm\n"
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
		{"encrypted", no_argument, NULL, 'e'},
		{"help", no_argument, NULL, 'h'},
		{"md5", no_argument, NULL, 'm'},
#ifdef USE_SHA_CRYPT
		{"sha-rounds", required_argument, NULL, 's'},
#endif
		{NULL, 0, NULL, '\0'}
	};

	while ((c = getopt_long (argc, argv,
#ifdef USE_SHA_CRYPT
	                         "c:ehms:",
#else
	                         "c:ehm",
#endif
	                         long_options, &option_index)) != -1) {
		switch (c) {
		case 'c':
			cflg = 1;
			crypt_method = optarg;
			break;
		case 'e':
			eflg = 1;
			break;
		case 'h':
			usage ();
			break;
		case 'm':
			md5flg = 1;
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

	if ((eflg && (md5flg || cflg)) ||
	    (md5flg && cflg)) {
		fprintf (stderr,
		         _("%s: the -c, -e, and -m flags are exclusive\n"),
		         Prog);
		usage ();
	}

	if (cflg) {
		if (   0 != strcmp (crypt_method, "DES")
		    && 0 != strcmp (crypt_method, "MD5")
		    && 0 != strcmp (crypt_method, "NONE")
#ifdef USE_SHA_CRYPT
		    && 0 != strcmp (crypt_method, "SHA256")
		    && 0 != strcmp (crypt_method, "SHA512")
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
	pam_handle_t *pamh = NULL;
	int retval = PAM_SUCCESS;

	struct passwd *pampw;
	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("chgpasswd", pampw->pw_name, &conv, &pamh);
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
		fprintf (stderr, _("%s: can't lock group file\n"), Prog);
		exit (1);
	}
	if (gr_open (O_RDWR) == 0) {
		fprintf (stderr, _("%s: can't open group file\n"), Prog);
		gr_unlock ();
		exit (1);
	}

#ifdef SHADOWGRP
	/* Do the same for the shadowed database, if it exist */
	if (is_shadow_grp) {
		if (sgr_lock () == 0) {
			fprintf (stderr, _("%s: can't lock gshadow file\n"),
			         Prog);
			gr_unlock ();
			exit (1);
		}
		if (sgr_open (O_RDWR) == 0) {
			fprintf (stderr, _("%s: can't open shadow file\n"),
			         Prog);
			gr_unlock ();
			sgr_unlock ();
			exit (1);
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
			         _("%s: error updating gshadow file\n"), Prog);
			gr_unlock ();
			exit (1);
		}
		sgr_unlock ();
	}
#endif

	if (gr_close () == 0) {
		fprintf (stderr, _("%s: error updating group file\n"), Prog);
		exit (1);
	}
	gr_unlock ();
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
	int ok;

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	process_flags(argc, argv);

	check_perms ();

	is_shadow_grp = sgr_file_present ();

	open_files ();

	/*
	 * Read each line, separating the group name from the password. The
	 * group entry for each group will be looked up in the appropriate
	 * file (gshadow or group) and the password changed.
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
		if (!eflg &&
		    (NULL == crypt_method ||
		     0 != strcmp(crypt_method, "NONE"))) {
			void *arg = NULL;
			if (md5flg) {
				crypt_method = "MD5";
			} else if (crypt_method != NULL) {
				if (sflg) {
					arg = &sha_rounds;
				}
			} else {
				crypt_method = NULL;
			}
			cp = pw_encrypt (newpwd,
			                 crypt_make_salt(crypt_method, arg));
		}

		/*
		 * Get the group file entry for this group. The group must
		 * already exist.
		 */
		gr = gr_locate (name);
		if (NULL == gr) {
			fprintf (stderr,
			         _("%s: line %d: unknown group %s\n"), Prog,
			         line, name);
			errors++;
			continue;
		}
#ifdef SHADOWGRP
		if (is_shadow_grp) {
			sg = sgr_locate (name);
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
		} else
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
			ok = sgr_update (&newsg);
		} else
#endif
		{
			ok = gr_update (&newgr);
		}

		if (!ok) {
			fprintf (stderr,
			         _
			         ("%s: line %d: cannot update group entry\n"),
			         Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes to be
	 * aborted. Unlocking the group file will cause all of the
	 * changes to be ignored. Otherwise the file is closed, causing the
	 * changes to be written out all at once, and then unlocked
	 * afterwards.
	 */
	if (errors) {
		fprintf (stderr,
		         _("%s: error detected, changes ignored\n"), Prog);
#ifdef SHADOWGRP
		if (is_shadow_grp) {
			sgr_unlock ();
		}
#endif
		gr_unlock ();
		exit (1);
	}

	close_files ();

	nscd_flush_cache ("group");

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	return (0);
}

