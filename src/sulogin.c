/*
 * SPDX-FileCopyrightText: 1989 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2002 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2010, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "defines.h"
#include "getdef.h"
#include "prototypes.h"
#include "pwauth.h"
/*@-exitarg@*/
#include "exitcodes.h"
#include "shadowlog.h"

/*
 * Global variables
 */
static const char Prog[] = "sulogin";

static char pass[BUFSIZ];

static struct passwd pwent;

extern char **newenvp;
extern size_t newenvc;

#ifndef	ALARM
#define	ALARM	60
#endif

/* local function prototypes */
static void catch_signals (int);

static void catch_signals (unused int sig)
{
	_exit (1);
}

 /*ARGSUSED*/ int main (int argc, char **argv)
{
#ifndef USE_PAM
	const char *env;
#endif				/* !USE_PAM */
	char **envp = environ;
	TERMIO termio;
	int err = 0;

	tcgetattr (0, &termio);
	termio.c_iflag |= (ICRNL | IXON);
	termio.c_oflag |= (CREAD);
	termio.c_lflag |= (ECHO | ECHOE | ECHOK | ICANON | ISIG);
	tcsetattr (0, TCSANOW, &termio);

	log_set_progname(Prog);
	log_set_logfd(stderr);
	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	initenv ();
	if (argc > 1) {
		close (0);
		close (1);
		close (2);

		if (open (argv[1], O_RDWR) >= 0) {
			dup (0);
			dup (0);
		} else {
			exit (1);
		}
	}
	if (access (PASSWD_FILE, F_OK) == -1) {	/* must be a password file! */
		(void) puts (_("No password file"));
		exit (1);
	}
#if !defined(DEBUG) && defined(SULOGIN_ONLY_INIT)
	if (getppid () != 1) {	/* parent must be INIT */
		exit (1);
	}
#endif
	if ((isatty (0) == 0) || (isatty (1) == 0) || (isatty (2) == 0)) {
		exit (1);	/* must be a terminal */
	}
	/* If we were init, we need to start a new session */
	if (getppid() == 1) {
		setsid();
		if (ioctl(0, TIOCSCTTY, 1) != 0) {
			(void) fputs (_("TIOCSCTTY failed"), stderr);
		}
	}
	while (NULL != *envp) {		/* add inherited environment, */
		addenv (*envp, NULL);	/* some variables change later */
		envp++;
	}

#ifndef USE_PAM
	env = getdef_str ("ENV_TZ");
	if (NULL != env) {
		addenv (('/' == *env) ? tz (env) : env, NULL);
	}
	env = getdef_str ("ENV_HZ");
	if (NULL != env) {
		addenv (env, NULL);	/* set the default $HZ, if one */
	}
#endif				/* !USE_PAM */

	(void) signal (SIGALRM, catch_signals);	/* exit if the timer expires */
	(void) alarm (ALARM);		/* only wait so long ... */

	while (true) {		/* repeatedly get login/password pairs */
		char *cp;
		pw_entry ("root", &pwent);	/* get entry from password file */
		if (pwent.pw_name == NULL) {
			/*
			 * Fail secure
			 */
			(void) puts (_("No password entry for 'root'"));
			exit (1);
		}

		/*
		 * Here we prompt for the root password, or if no password
		 * is given we just exit.
		 */

		/* get a password for root */
		cp = agetpass (_(
"\n"
"Type control-d to proceed with normal startup,\n"
"(or give root password for system maintenance):"));
		/*
		 * XXX - can't enter single user mode if root password is
		 * empty.  I think this doesn't happen very often :-). But
		 * it will work with standard getpass() (no NULL on EOF).
		 * --marekm
		 */
		if ((NULL == cp) || ('\0' == *cp)) {
			erase_pass (cp);
			(void) puts ("");
#ifdef	TELINIT
			execl (PATH_TELINIT, "telinit", RUNLEVEL, (char *) NULL);
#endif
			exit (0);
		}
		STRFCPY (pass, cp);
		erase_pass (cp);

		if (valid (pass, &pwent)) {	/* check encrypted passwords ... */
			break;	/* ... encrypted passwords matched */
		}

		sleep (2);
		(void) puts (_("Login incorrect"));
	}
	memzero (pass, sizeof pass);
	(void) alarm (0);
	(void) signal (SIGALRM, SIG_DFL);
	environ = newenvp;	/* make new environment active */

	(void) puts (_("Entering System Maintenance Mode"));

	/* exec the shell finally. */
	err = shell (pwent.pw_shell, NULL, environ);

	return ((err == ENOENT) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
}

