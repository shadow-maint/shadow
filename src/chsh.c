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
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "rcsid.h"
RCSID(PKG_VER "$Id: chsh.c,v 1.15 1999/07/09 18:02:43 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "prototypes.h"
#include "defines.h"

#include <pwd.h>
#include "pwio.h"
#include "getdef.h"
#include "pwauth.h"

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef USE_PAM
#include "pam_defs.h"
#endif

#ifndef SHELLS_FILE
#define SHELLS_FILE "/etc/shells"
#endif

/*
 * Global variables.
 */

static char *Prog;			/* Program name */
static int amroot;				/* Real UID is root */
static char loginsh[BUFSIZ];		/* Name of new login shell */

/*
 * External identifiers
 */

extern	int	optind;
extern	char	*optarg;
#ifdef	NDBM
extern	int	pw_dbm_mode;
#endif

/*
 * #defines for messages.  This facilitates foreign language conversion
 * since all messages are defined right here.
 */

#define WRONGPWD2	"incorrect password for `%s'"
#define	NOPERM2		"can't change shell for `%s'\n"
#define	PWDBUSY2	"can't lock /etc/passwd\n"
#define	OPNERROR2	"can't open /etc/passwd\n"
#define	UPDERROR2	"error updating passwd entry\n"
#define	DBMERROR2	"error updating DBM passwd entry.\n"
#define	NOTROOT2	"can't setuid(0).\n"
#define	CLSERROR2	"can't rewrite /etc/passwd.\n"
#define	UNLKERROR2	"can't unlock /etc/passwd.\n"
#define	CHGSHELL	"changed user `%s' shell to `%s'\n"

/* local function prototypes */
static void usage P_((void));
static void new_fields P_((void));
static int restricted_shell P_((const char *));
int main P_((int, char **));

/*
 * usage - print command line syntax and exit
 */

static void
usage(void)
{
	fprintf(stderr, _("Usage: %s [ -s shell ] [ name ]\n"), Prog);
	exit(1);
}

/*
 * new_fields - change the user's login shell information interactively
 *
 * prompt the user for the login shell and change it according to the
 * response, or leave it alone if nothing was entered.
 */

static void
new_fields(void)
{
	printf(_("Enter the new value, or press return for the default\n"));
	change_field(loginsh, sizeof loginsh, _("Login Shell"));
}

/*
 * restricted_shell - return true if the named shell begins with 'r' or 'R'
 *
 * If the first letter of the filename is 'r' or 'R', the shell is
 * considered to be restricted.
 */

static int
restricted_shell(const char *sh)
{
#if 0
	char *cp = Basename((char *) sh);
	return *cp == 'r' || *cp == 'R';
#else
	/*
	 * Shells not listed in /etc/shells are considered to be
	 * restricted.  Changed this to avoid confusion with "rc"
	 * (the plan9 shell - not restricted despite the name
	 * starting with 'r').  --marekm
	 */
	return !check_shell(sh);
#endif
}


/*
 * chsh - this command controls changes to the user's shell
 *
 *	The only supported option is -s which permits the
 *	the login shell to be set from the command line.
 */

int
main(int argc, char **argv)
{
	char	*user;			/* User name                         */
	int	flag;			/* Current command line flag         */
	int	sflg = 0;		/* -s - set shell from command line  */
	const struct passwd *pw;	/* Password entry from /etc/passwd   */
	struct	passwd	pwent;		/* New password entry                */

	sanitize_env();

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	/*
	 * This command behaves different for root and non-root
	 * users.
	 */

	amroot = getuid () == 0;
#ifdef	NDBM
	pw_dbm_mode = O_RDWR;
#endif

	/*
	 * Get the program name.  The program name is used as a
	 * prefix to most error messages.  It is also used as input
	 * to the openlog() function for error logging.
	 */

	Prog = Basename(argv[0]);

	openlog("chsh", LOG_PID, LOG_AUTH);

	/*
	 * There is only one option, but use getopt() anyway to
	 * keep things consistent.
	 */

	while ((flag = getopt (argc, argv, "s:")) != EOF) {
		switch (flag) {
			case 's':
				sflg++;
				STRFCPY(loginsh, optarg);
				break;
			default:
				usage ();
		}
	}

	/*
	 * There should be only one remaining argument at most
	 * and it should be the user's name.
	 */

	if (argc > optind + 1)
		usage ();

	/*
	 * Get the name of the user to check.  It is either
	 * the command line name, or the name getlogin()
	 * returns.
	 */

	if (optind < argc) {
		user = argv[optind];
		pw = getpwnam(user);
		if (!pw) {
			fprintf(stderr,
				_("%s: Unknown user %s\n"),
				Prog, user);
			exit(1);
		}
	} else {
		pw = get_my_pwent();
		if (!pw) {
			fprintf(stderr,
				_("%s: Cannot determine your user name.\n"),
				Prog);
			exit(1);
		}
		user = xstrdup(pw->pw_name);
	}

#ifdef	USE_NIS
	/*
	 * Now we make sure this is a LOCAL password entry for
	 * this user ...
	 */

	if (__ispwNIS ()) {
		char	*nis_domain;
		char	*nis_master;

		fprintf(stderr,
			_("%s: cannot change user `%s' on NIS client.\n"),
			Prog, user);

		if (! yp_get_default_domain (&nis_domain) &&
				! yp_master (nis_domain, "passwd.byname",
				&nis_master)) {
			fprintf(stderr,
				_("%s: `%s' is the NIS master for this client.\n"),
				Prog, nis_master);
		}
		exit (1);
	}
#endif

	/*
	 * Non-privileged users are only allowed to change the
	 * shell if the UID of the user matches the current
	 * real UID.
	 */

	if (! amroot && pw->pw_uid != getuid()) {
		SYSLOG((LOG_WARN, NOPERM2, user));
		closelog();
		fprintf(stderr, _("You may not change the shell for %s.\n"),
			user);
		exit(1);
	}

	/*
	 * Non-privileged users are only allowed to change the
	 * shell if it is not a restricted one.
	 */

	if (! amroot && restricted_shell(pw->pw_shell)) {
		SYSLOG((LOG_WARN, NOPERM2, user));
		closelog();
		fprintf(stderr, _("You may not change the shell for %s.\n"),
			user);
		exit(1);
	}

	/*
 	* Non-privileged users are optionally authenticated
 	* (must enter the password of the user whose information
 	* is being changed) before any changes can be made.
 	* Idea from util-linux chfn/chsh.  --marekm
 	*/

	if (!amroot && getdef_bool("CHFN_AUTH"))
		passwd_check(pw->pw_name, pw->pw_passwd, "chsh");

	/*
	 * Now get the login shell.  Either get it from the password
	 * file, or use the value from the command line.
	 */

	if (! sflg)
		STRFCPY(loginsh, pw->pw_shell);

	/*
	 * If the login shell was not set on the command line,
	 * let the user interactively change it.
	 */

	if (! sflg) {
		printf(_("Changing the login shell for %s\n"), user);
		new_fields();
	}

	/*
	 * Check all of the fields for valid information.  The shell
	 * field may not contain any illegal characters.  Non-privileged
	 * users are restricted to using the shells in /etc/shells.
	 * The shell must be executable by the user.
	 */

	if (valid_field (loginsh, ":,=")) {
		fprintf(stderr, _("%s: Invalid entry: %s\n"), Prog, loginsh);
		closelog();
		exit(1);
	}
	if (!amroot && (!check_shell(loginsh) || access(loginsh, X_OK) != 0)) {
		fprintf(stderr, _("%s is an invalid shell.\n"), loginsh);
		closelog();
		exit(1);
	}

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals.  Any
	 * keyboard signals are set to be ignored.
	 */

	if (setuid(0)) {
		SYSLOG((LOG_ERR, NOTROOT2));
		closelog();
		fprintf (stderr, _("Cannot change ID to root.\n"));
		exit(1);
	}
	pwd_init();

	/*
	 * The passwd entry is now ready to be committed back to
	 * the password file.  Get a lock on the file and open it.
	 */

	if (!pw_lock()) {
		SYSLOG((LOG_WARN, PWDBUSY2));
		closelog();
		fprintf(stderr,
			_("Cannot lock the password file; try again later.\n"));
		exit(1);
	}
	if (! pw_open (O_RDWR)) {
		SYSLOG((LOG_ERR, OPNERROR2));
		closelog();
		fprintf(stderr, _("Cannot open the password file.\n"));
		pw_unlock();
		exit(1);
	}

	/*
	 * Get the entry to update using pw_locate() - we want the real
	 * one from /etc/passwd, not the one from getpwnam() which could
	 * contain the shadow password if (despite the warnings) someone
	 * enables AUTOSHADOW (or SHADOW_COMPAT in libc).  --marekm
	 */
	pw = pw_locate(user);
	if (!pw) {
		pw_unlock();
		fprintf(stderr,
			_("%s: %s not found in /etc/passwd\n"), Prog, user);
		exit(1);
	}

	/*
	 * Make a copy of the entry, then change the shell field.  The other
	 * fields remain unchanged.
	 */
	pwent = *pw;
	pwent.pw_shell = loginsh;

	/*
	 * Update the passwd file entry.  If there is a DBM file,
	 * update that entry as well.
	 */

	if (!pw_update(&pwent)) {
		SYSLOG((LOG_ERR, UPDERROR2));
		closelog();
		fprintf(stderr, _("Error updating the password entry.\n"));
		pw_unlock();
		exit(1);
	}
#ifdef NDBM
	if (pw_dbm_present() && ! pw_dbm_update (&pwent)) {
		SYSLOG((LOG_ERR, DBMERROR2));
		closelog();
		fprintf (stderr, _("Error updating the DBM password entry.\n"));
		pw_unlock();
		exit(1);
	}
	endpwent();
#endif

	/*
	 * Changes have all been made, so commit them and unlock the
	 * file.
	 */

	if (!pw_close()) {
		SYSLOG((LOG_ERR, CLSERROR2));
		closelog();
		fprintf(stderr, _("Cannot commit password file changes.\n"));
		pw_unlock();
		exit(1);
	}
	if (!pw_unlock()) {
		SYSLOG((LOG_ERR, UNLKERROR2));
		closelog();
		fprintf(stderr, _("Cannot unlock the password file.\n"));
		exit(1);
	}
	SYSLOG((LOG_INFO, CHGSHELL, user, loginsh));
	closelog();
	exit (0);
}
