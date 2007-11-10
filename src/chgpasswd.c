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

#ident "$Id: chgpasswd.c,v 1.5 2006/06/20 20:00:04 kloczek Exp $"

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
static int eflg = 0;
static int md5flg = 0;

#ifdef SHADOWGRP
static int is_shadow_grp;
#endif

/* local function prototypes */
static void usage (void);

/*
 * usage - display usage message and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: chgpasswd [options]\n"
			   "\n"
			   "Options:\n"
			   "  -e, --encrypted	supplied passwords are encrypted\n"
			   "  -h, --help		display this help message and exit\n"
			   "  -m, --md5		use MD5 encryption instead DES when the supplied\n"
			   "			passwords are not encrypted\n"
			   "\n"));
	exit (1);
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
	int errors = 0;
	int line = 0;
	int ok;

#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	struct passwd *pampw;
	int retval;
#endif
#ifndef SHADOWGRP
	struct group newgr;
#endif

	Prog = Basename (argv[0]);

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	{
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"encrypted", no_argument, NULL, 'e'},
			{"help", no_argument, NULL, 'h'},
			{"md5", no_argument, NULL, 'm'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "ehm", long_options,
				     &option_index)) != -1) {
			switch (c) {
			case 'e':
				eflg = 1;
				break;
			case 'h':
				usage ();
				break;
			case 'm':
				md5flg = 1;
				break;
			case 0:
				/* long option */
				break;
			default:
				usage ();
				break;
			}
		}
	}

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	pampw = getpwuid (getuid ());
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("chpasswd", pampw->pw_name, &conv, &pamh);
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
	 * Lock the group file and open it for reading. This will bring
	 * all of the entries into memory where they may be updated.
	 */
	if (!gr_lock ()) {
		fprintf (stderr, _("%s: can't lock group file\n"), Prog);
		exit (1);
	}
	if (!gr_open (O_RDWR)) {
		fprintf (stderr, _("%s: can't open group file\n"), Prog);
		gr_unlock ();
		exit (1);
	}
#ifdef SHADOWGRP
	is_shadow_grp = sgr_file_present ();
	if (is_shadow_grp) {
		if (!sgr_lock ()) {
			fprintf (stderr, _("%s: can't lock gshadow file\n"),
				 Prog);
			gr_unlock ();
			exit (1);
		}
		if (!sgr_open (O_RDWR)) {
			fprintf (stderr, _("%s: can't open shadow file\n"),
				 Prog);
			gr_unlock ();
			sgr_unlock ();
			exit (1);
		}
	}
#endif

	/*
	 * Read each line, separating the group name from the password. The
	 * password entry for each group will be looked up in the appropriate
	 * file (gshadow or group) and the password changed.
	 */
	while (fgets (buf, sizeof buf, stdin) != (char *) 0) {
		line++;
		if ((cp = strrchr (buf, '\n'))) {
			*cp = '\0';
		} else {
			fprintf (stderr, _("%s: line %d: line too long\n"),
				 Prog, line);
			errors++;
			continue;
		}

		/*
		 * The groupname is the first field. It is separated from the
		 * password with a ":" character which is replaced with a
		 * NUL to give the new password. The new password will then
		 * be encrypted in the normal fashion with a new salt
		 * generated, unless the '-e' is given, in which case it is
		 * assumed to already be encrypted.
		 */

		name = buf;
		if ((cp = strchr (name, ':'))) {
			*cp++ = '\0';
		} else {
			fprintf (stderr,
				 _("%s: line %d: missing new password\n"),
				 Prog, line);
			errors++;
			continue;
		}
		newpwd = cp;
		if (!eflg) {
			if (md5flg) {
				char tmp[12];
				char salt[15] = "";

				strcat (tmp, crypt_make_salt ());
				if (!strncmp (tmp, "$1$", 3))
					strcat (salt, "$1$");
				strcat (salt, tmp);
				cp = pw_encrypt (newpwd, salt);
			} else
				cp = pw_encrypt (newpwd, crypt_make_salt ());
		}

		/*
		 * Get the password file entry for this user. The user must
		 * already exist.
		 */
		gr = gr_locate (name);
		if (!gr) {
			fprintf (stderr,
				 _("%s: line %d: unknown group %s\n"), Prog,
				 line, name);
			errors++;
			continue;
		}
#ifdef SHADOWGRP
		sg = sgr_locate (name);
#endif

		/*
		 * The freshly encrypted new password is merged into the
		 * user's password file entry and the last password change
		 * date is set to the current date.
		 */
#ifdef SHADOWGRP
		newsg = *sg;
		newsg.sg_passwd = cp;
#else
		newgr = *gr;
		newgr.gr_passwd = cp;
#endif

		/* 
		 * The updated password file entry is then put back and will
		 * be written to the password file later, after all the
		 * other entries have been updated as well.
		 */
#ifdef SHADOWGRP
		ok = sgr_update (&newsg);
#else
		ok = gr_update (&newgr);
#endif

		if (!ok) {
			fprintf (stderr,
				 _
				 ("%s: line %d: cannot update password entry\n"),
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
#ifdef SHADOWGRP
		if (is_shadow_grp)
			sgr_unlock ();
#endif
		gr_unlock ();
		exit (1);
	}
#ifdef SHADOWGRP
	if (is_shadow_grp) {
		if (!sgr_close ()) {
			fprintf (stderr,
				 _("%s: error updating shadow file\n"), Prog);
			gr_unlock ();
			exit (1);
		}
		sgr_unlock ();
	}
#endif
	if (!gr_close ()) {
		fprintf (stderr, _("%s: error updating password file\n"), Prog);
		exit (1);
	}

	nscd_flush_cache ("group");

	gr_unlock ();

#ifdef USE_PAM
	if (retval == PAM_SUCCESS)
		pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	return (0);
}
