/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#include <selinux/av_permissions.h>
#endif
#include "defines.h"
#include "exitcodes.h"
#include "getdef.h"
#include "nscd.h"
#include "prototypes.h"
#include "pwauth.h"
#include "pwio.h"
#ifdef USE_PAM
#include "pam_defs.h"
#endif
#ifndef SHELLS_FILE
#define SHELLS_FILE "/etc/shells"
#endif
/*
 * Global variables
 */
static char *Prog;		/* Program name */
static int amroot;		/* Real UID is root */
static char loginsh[BUFSIZ];	/* Name of new login shell */
/* command line options */
static int sflg = 0;		/* -s - set shell from command line  */
#ifdef USE_PAM
static pam_handle_t *pamh = NULL;
#endif

/* external identifiers */

/* local function prototypes */
static void usage (void);
static void new_fields (void);
static int check_shell (const char *);
static int restricted_shell (const char *);
static void process_flags (int argc, char **argv);
static void check_perms (const struct passwd *pw);
static void update_shell (const char *user, char *loginsh);

/*
 * usage - print command line syntax and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: chsh [options] [LOGIN]\n"
	                   "\n"
	                   "Options:\n"
	                   "  -h, --help                    display this help message and exit\n"
	                   "  -s, --shell SHELL             new login shell for the user account\n"
	                   "\n"));
	exit (E_USAGE);
}

/*
 * new_fields - change the user's login shell information interactively
 *
 * prompt the user for the login shell and change it according to the
 * response, or leave it alone if nothing was entered.
 */
static void new_fields (void)
{
	printf (_("Enter the new value, or press ENTER for the default\n"));
	change_field (loginsh, sizeof loginsh, _("Login Shell"));
}

/*
 * restricted_shell - return true if the named shell begins with 'r' or 'R'
 *
 * If the first letter of the filename is 'r' or 'R', the shell is
 * considered to be restricted.
 */
static int restricted_shell (const char *sh)
{
	/*
	 * Shells not listed in /etc/shells are considered to be restricted.
	 * Changed this to avoid confusion with "rc" (the plan9 shell - not
	 * restricted despite the name starting with 'r').  --marekm
	 */
	return !check_shell (sh);
}

/*
 * check_shell - see if the user's login shell is listed in /etc/shells
 *
 * The /etc/shells file is read for valid names of login shells.  If the
 * /etc/shells file does not exist the user cannot set any shell unless
 * they are root.
 *
 * If getusershell() is available (Linux, *BSD, possibly others), use it
 * instead of re-implementing it.
 */
static int check_shell (const char *sh)
{
	char *cp;
	int found = 0;

#ifndef HAVE_GETUSERSHELL
	char buf[BUFSIZ];
	FILE *fp;
#endif

#ifdef HAVE_GETUSERSHELL
	setusershell ();
	while ((cp = getusershell ())) {
		if (*cp == '#') {
			continue;
		}

		if (strcmp (cp, sh) == 0) {
			found = 1;
			break;
		}
	}
	endusershell ();
#else
	fp = fopen (SHELLS_FILE, "r");
	if (NULL == fp) {
		return 0;
	}

	while (fgets (buf, sizeof (buf), fp)) {
		cp = strrchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		}

		if (buf[0] == '#') {
			continue;
		}

		if (strcmp (buf, sh) == 0) {
			found = 1;
			break;
		}
	}
	fclose (fp);
#endif
	return found;
}

/*
 *  * process_flags - parse the command line options
 *
 *	It will not return if an error is encountered.
 */
static void process_flags (int argc, char **argv)
{
	int option_index = 0;
	int c;
	static struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"shell", required_argument, NULL, 's'},
		{NULL, 0, NULL, '\0'}
	};

	while ((c =
		getopt_long (argc, argv, "hs:", long_options,
		             &option_index)) != -1) {
		switch (c) {
		case 'h':
			usage ();
			break;
		case 's':
			sflg++;
			STRFCPY (loginsh, optarg);
			break;
		default:
			usage ();
		}
	}

	/*
	 * There should be only one remaining argument at most and it should
	 * be the user's name.
	 */
	if (argc > (optind + 1)) {
		usage ();
	}
}

/*
 * check_perms - check if the caller is allowed to add a group
 *
 *	Non-root users are only allowed to change their shell, if their current
 *	shell is not a restricted shell.
 *
 *	Non-root users must be authenticated.
 *
 *	It will not return if the user is not allowed.
 */
static void check_perms (const struct passwd *pw)
{
#ifdef USE_PAM
	int retval;
	struct passwd *pampw;
#endif

	/*
	 * Non-privileged users are only allowed to change the shell if the
	 * UID of the user matches the current real UID.
	 */
	if (!amroot && pw->pw_uid != getuid ()) {
		SYSLOG ((LOG_WARN, "can't change shell for `%s'", pw->pw_name));
		closelog ();
		fprintf (stderr,
		         _("You may not change the shell for %s.\n"),
		         pw->pw_name);
		exit (1);
	}

	/*
	 * Non-privileged users are only allowed to change the shell if it
	 * is not a restricted one.
	 */
	if (!amroot && restricted_shell (pw->pw_shell)) {
		SYSLOG ((LOG_WARN, "can't change shell for `%s'", pw->pw_name));
		closelog ();
		fprintf (stderr,
		         _("You may not change the shell for %s.\n"),
		         pw->pw_name);
		exit (1);
	}
#ifdef WITH_SELINUX
	/*
	 * If the UID of the user does not match the current real UID,
	 * check if the change is allowed by SELinux policy.
	 */
	if ((pw->pw_uid != getuid ())
	    && (is_selinux_enabled () > 0)
	    && (selinux_check_passwd_access (PASSWD__CHSH) != 0)) {
		SYSLOG ((LOG_WARN, "can't change shell for `%s'", pw->pw_name));
		closelog ();
		fprintf (stderr,
		         _("You may not change the shell for %s.\n"),
		         pw->pw_name);
		exit (1);
	}
#endif

#ifndef USE_PAM
	/*
	 * Non-privileged users are optionally authenticated (must enter
	 * the password of the user whose information is being changed)
	 * before any changes can be made. Idea from util-linux
	 * chfn/chsh.  --marekm
	 */
	if (!amroot && getdef_bool ("CHSH_AUTH")) {
		passwd_check (pw->pw_name, pw->pw_passwd, "chsh");
        }

#else				/* !USE_PAM */
	retval = PAM_SUCCESS;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("chsh", pampw->pw_name, &conv, &pamh);
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
		exit (E_NOPERM);
	}
#endif				/* USE_PAM */
}

/*
 * update_shell - update the user's shell in the passwd database
 *
 *	Commit the user's entry after changing her shell field.
 *
 *	It will not return in case of error.
 */
static void update_shell (const char *user, char *newshell)
{
	const struct passwd *pw;	/* Password entry from /etc/passwd   */
	struct passwd pwent;		/* New password entry                */

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals. Any
	 * keyboard signals are set to be ignored.
	 */
	if (setuid (0) != 0) {
		SYSLOG ((LOG_ERR, "can't setuid(0)"));
		closelog ();
		fprintf (stderr, _("Cannot change ID to root.\n"));
		exit (1);
	}
	pwd_init ();

	/*
	 * The passwd entry is now ready to be committed back to
	 * the password file. Get a lock on the file and open it.
	 */
	if (pw_lock () == 0) {
		SYSLOG ((LOG_WARN, "can't lock /etc/passwd"));
		closelog ();
		fprintf (stderr,
		         _
		         ("Cannot lock the password file; try again later.\n"));
		exit (1);
	}
	if (pw_open (O_RDWR) == 0) {
		SYSLOG ((LOG_ERR, "can't open /etc/passwd"));
		closelog ();
		fprintf (stderr, _("Cannot open the password file.\n"));
		pw_unlock ();
		exit (1);
	}

	/*
	 * Get the entry to update using pw_locate() - we want the real
	 * one from /etc/passwd, not the one from getpwnam() which could
	 * contain the shadow password if (despite the warnings) someone
	 * enables AUTOSHADOW (or SHADOW_COMPAT in libc).  --marekm
	 */
	pw = pw_locate (user);
	if (NULL == pw) {
		pw_unlock ();
		fprintf (stderr,
		         _("%s: %s not found in /etc/passwd\n"), Prog, user);
		exit (1);
	}

	/*
	 * Make a copy of the entry, then change the shell field. The other
	 * fields remain unchanged.
	 */
	pwent = *pw;
	pwent.pw_shell = newshell;

	/*
	 * Update the passwd file entry. If there is a DBM file, update
	 * that entry as well.
	 */
	if (pw_update (&pwent) == 0) {
		SYSLOG ((LOG_ERR, "error updating passwd entry"));
		closelog ();
		fprintf (stderr, _("Error updating the password entry.\n"));
		pw_unlock ();
		exit (1);
	}

	/*
	 * Changes have all been made, so commit them and unlock the file.
	 */
	if (pw_close () == 0) {
		SYSLOG ((LOG_ERR, "can't rewrite /etc/passwd"));
		closelog ();
		fprintf (stderr, _("Cannot commit password file changes.\n"));
		pw_unlock ();
		exit (1);
	}
	if (pw_unlock () == 0) {
		SYSLOG ((LOG_ERR, "can't unlock /etc/passwd"));
		closelog ();
		fprintf (stderr, _("Cannot unlock the password file.\n"));
		exit (1);
	}
}

/*
 * chsh - this command controls changes to the user's shell
 *
 *	The only supported option is -s which permits the the login shell to
 *	be set from the command line.
 */
int main (int argc, char **argv)
{
	char *user;		/* User name                         */
	const struct passwd *pw;	/* Password entry from /etc/passwd   */

	sanitize_env ();

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * This command behaves different for root and non-root users.
	 */
	amroot = getuid () == 0;

	/*
	 * Get the program name. The program name is used as a prefix to
	 * most error messages.
	 */
	Prog = Basename (argv[0]);

	OPENLOG ("chsh");

	/* parse the command line options */
	process_flags (argc, argv);

	/*
	 * Get the name of the user to check. It is either the command line
	 * name, or the name getlogin() returns.
	 */
	if (optind < argc) {
		user = argv[optind];
		pw = xgetpwnam (user);
		if (!pw) {
			fprintf (stderr,
			         _("%s: unknown user %s\n"), Prog, user);
			exit (1);
		}
	} else {
		pw = get_my_pwent ();
		if (!pw) {
			fprintf (stderr,
			         _
			         ("%s: Cannot determine your user name.\n"),
			         Prog);
			exit (1);
		}
		user = xstrdup (pw->pw_name);
	}

#ifdef	USE_NIS
	/*
	 * Now we make sure this is a LOCAL password entry for this user ...
	 */
	if (__ispwNIS ()) {
		char *nis_domain;
		char *nis_master;

		fprintf (stderr,
		         _("%s: cannot change user '%s' on NIS client.\n"),
		         Prog, user);

		if (!yp_get_default_domain (&nis_domain) &&
		    !yp_master (nis_domain, "passwd.byname", &nis_master)) {
			fprintf (stderr,
			         _
			         ("%s: '%s' is the NIS master for this client.\n"),
			         Prog, nis_master);
		}
		exit (1);
	}
#endif

	check_perms (pw);

	/*
	 * Now get the login shell. Either get it from the password
	 * file, or use the value from the command line.
	 */
	if (!sflg) {
		STRFCPY (loginsh, pw->pw_shell);
	}

	/*
	 * If the login shell was not set on the command line, let the user
	 * interactively change it.
	 */
	if (!sflg) {
		printf (_("Changing the login shell for %s\n"), user);
		new_fields ();
	}

	/*
	 * Check all of the fields for valid information. The shell
	 * field may not contain any illegal characters. Non-privileged
	 * users are restricted to using the shells in /etc/shells.
	 * The shell must be executable by the user.
	 */
	if (valid_field (loginsh, ":,=")) {
		fprintf (stderr, _("%s: Invalid entry: %s\n"), Prog, loginsh);
		closelog ();
		exit (1);
	}
	if (!amroot && (!check_shell (loginsh) || access (loginsh, X_OK) != 0)) {
		fprintf (stderr, _("%s is an invalid shell.\n"), loginsh);
		closelog ();
		exit (1);
	}

	update_shell (user, loginsh);

	SYSLOG ((LOG_INFO, "changed user `%s' shell to `%s'", user, loginsh));

	nscd_flush_cache ("passwd");

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	closelog ();
	exit (E_SUCCESS);
}

