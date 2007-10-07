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
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * chpasswd - update passwords in batch
 *
 *	chpasswd reads standard input for a list of colon separated
 *	user names and new passwords.  the appropriate password
 *	files are updated to reflect the changes.  because the
 *	changes are made in a batch fashion, the user must run
 *	the mkpasswd command after this command terminates since
 *	no password updates occur until the very end.
 *
 * 1997/07/29: Modified to take "-e" argument which specifies that
 *             the passwords have already been encrypted.
 *             -- Jay Soffian <jay@lw.net>
 */

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: chpasswd.c,v 1.12 2000/10/09 19:02:20 kloczek Exp $")

#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <fcntl.h>
#include "pwio.h"
#ifdef	SHADOWPWD
#include "shadowio.h"
#endif

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif /* USE_PAM */

static char *Prog;
static int eflg = 0;
#ifdef SHADOWPWD
static int is_shadow_pwd;
#endif

extern	char	*l64a();

/* local function prototypes */
static void usage(void);

/*
 * usage - display usage message and exit
 */

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [-e]\n"), Prog);
	exit(1);
}

#ifdef USE_PAM
static struct pam_conv conv = {
    misc_conv,
    NULL
};
#endif /* USE_PAM */

int
main(int argc, char **argv)
{
	char	buf[BUFSIZ];
	char	*name;
	char	*newpwd;
	char	*cp;
#ifdef	SHADOWPWD
	const struct spwd *sp;
	struct	spwd	newsp;
#endif
	const struct passwd *pw;
	struct	passwd	newpw;
#ifdef ATT_AGE
	char	newage[5];
#endif
	int	errors = 0;
	int	line = 0;
	long	now = time ((long *) 0) / (24L*3600L);
	int ok;
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	struct passwd *pampw;
	int retval;
#endif

	Prog = Basename(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	pampw = getpwuid(getuid());
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start("shadow", pampw->pw_name, &conv, &pamh);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate(pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end(pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt(pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end(pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (1);
	}
#endif /* USE_PAM */

	/* XXX - use getopt() */
	if (!(argc == 1 || (argc == 2 && !strcmp(argv[1], "-e"))))
		usage();
	if (argc == 2)
		eflg = 1;

	/*
	 * Lock the password file and open it for reading.  This will
	 * bring all of the entries into memory where they may be
	 * updated.
	 */

	if (!pw_lock()) {
		fprintf(stderr, _("%s: can't lock password file\n"), Prog);
		exit(1);
	}
	if (! pw_open (O_RDWR)) {
		fprintf(stderr, _("%s: can't open password file\n"), Prog);
		pw_unlock();
		exit(1);
	}
#ifdef SHADOWPWD
	is_shadow_pwd = spw_file_present();
	if (is_shadow_pwd) {
		if (!spw_lock()) {
			fprintf(stderr, _("%s: can't lock shadow file\n"), Prog);
			pw_unlock();
			exit(1);
		}
		if (!spw_open(O_RDWR)) {
			fprintf(stderr, _("%s: can't open shadow file\n"), Prog);
			pw_unlock();
			spw_unlock();
			exit(1);
		}
	}
#endif

	/*
	 * Read each line, separating the user name from the password.
	 * The password entry for each user will be looked up in the
	 * appropriate file (shadow or passwd) and the password changed.
	 * For shadow files the last change date is set directly, for
	 * passwd files the last change date is set in the age only if
	 * aging information is present.
	 */

	while (fgets (buf, sizeof buf, stdin) != (char *) 0) {
		line++;
		if ((cp = strrchr (buf, '\n'))) {
			*cp = '\0';
		} else {
			fprintf(stderr, _("%s: line %d: line too long\n"),
				Prog, line);
			errors++;
			continue;
		}

		/*
		 * The username is the first field.  It is separated
		 * from the password with a ":" character which is
		 * replaced with a NUL to give the new password.  The
		 * new password will then be encrypted in the normal
		 * fashion with a new salt generated, unless the '-e'
		 * is given, in which case it is assumed to already be
		 * encrypted.
		 */

		name = buf;
		if ((cp = strchr (name, ':'))) {
			*cp++ = '\0';
		} else {
			fprintf(stderr, _("%s: line %d: missing new password\n"),
				Prog, line);
			errors++;
			continue;
		}
		newpwd = cp;
		if (!eflg)
			cp = pw_encrypt(newpwd, crypt_make_salt());

		/*
		 * Get the password file entry for this user.  The user
		 * must already exist.
		 */

		pw = pw_locate(name);
		if (!pw) {
			fprintf (stderr, _("%s: line %d: unknown user %s\n"),
				Prog, line, name);
			errors++;
			continue;
		}

#ifdef SHADOWPWD
		if (is_shadow_pwd)
			sp = spw_locate(name);
		else
			sp = NULL;
#endif

		/*
		 * The freshly encrypted new password is merged into
		 * the user's password file entry and the last password
		 * change date is set to the current date.
		 */

#ifdef SHADOWPWD
		if (sp) {
			newsp = *sp;
			newsp.sp_pwdp = cp;
			newsp.sp_lstchg = now;
		} else
#endif
		{
			newpw = *pw;
			newpw.pw_passwd = cp;
#ifdef	ATT_AGE
			if (newpw.pw_age[0]) {
				strcpy(newage, newpw.pw_age);
				strcpy(newage + 2, l64a(now / 7));
				newpw.pw_age = newage;
			}
#endif
		}

		/* 
		 * The updated password file entry is then put back
		 * and will be written to the password file later, after
		 * all the other entries have been updated as well.
		 */

#ifdef SHADOWPWD
		if (sp)
			ok = spw_update(&newsp);
		else
#endif
			ok = pw_update(&newpw);

		if (!ok) {
			fprintf(stderr, _("%s: line %d: cannot update password entry\n"),
				Prog, line);
			errors++;
			continue;
		}
	}

	/*
	 * Any detected errors will cause the entire set of changes
	 * to be aborted.  Unlocking the password file will cause
	 * all of the changes to be ignored.  Otherwise the file is
	 * closed, causing the changes to be written out all at
	 * once, and then unlocked afterwards.
	 */

	if (errors) {
		fprintf(stderr, _("%s: error detected, changes ignored\n"), Prog);
#ifdef SHADOWPWD
		if (is_shadow_pwd)
			spw_unlock();
#endif
		pw_unlock();
		exit(1);
	}
#ifdef SHADOWPWD
	if (is_shadow_pwd) {
		if (!spw_close()) {
			fprintf(stderr, _("%s: error updating shadow file\n"), Prog);
			pw_unlock();
			exit(1);
		}
		spw_unlock();
	}
#endif
	if (!pw_close()) {
		fprintf(stderr, _("%s: error updating password file\n"), Prog);
		exit(1);
	}
	pw_unlock();

#ifdef USE_PAM
	if (retval == PAM_SUCCESS) {
		retval = pam_chauthtok(pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end(pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM chauthtok failed\n"), Prog);
		exit (1);
	}

	if (retval == PAM_SUCCESS)
		pam_end(pamh, PAM_SUCCESS);
#endif /* USE_PAM */

	return (0);
}
