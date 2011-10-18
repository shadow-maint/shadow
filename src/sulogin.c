/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2002 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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

/*
 * Global variables
 */
const char *Prog;

static char name[BUFSIZ];
static char pass[BUFSIZ];

static struct passwd pwent;

extern char **newenvp;
extern size_t newenvc;

extern char **environ;

#ifndef	ALARM
#define	ALARM	60
#endif

/* local function prototypes */
static RETSIGTYPE catch_signals (int);

static RETSIGTYPE catch_signals (unused int sig)
{
	exit (1);
}

/*
 * syslogd is usually not running at the time when sulogin is typically
 * called, cluttering the screen with unnecessary messages. Suggested by
 * Ivan Nejgebauer <ian@unsux.ns.ac.yu>.  --marekm
 */
#undef USE_SYSLOG

 /*ARGSUSED*/ int main (int argc, char **argv)
{
#ifndef USE_PAM
	const char *env;
#endif				/* !USE_PAM */
	char **envp = environ;
	TERMIO termio;
	int err = 0;

#ifdef	USE_TERMIO
	ioctl (0, TCGETA, &termio);
	termio.c_iflag |= (ICRNL | IXON);
	termio.c_oflag |= (OPOST | ONLCR);
	termio.c_cflag |= (CREAD);
	termio.c_lflag |= (ISIG | ICANON | ECHO | ECHOE | ECHOK);
	ioctl (0, TCSETAF, &termio);
#endif
#ifdef	USE_TERMIOS
	tcgetattr (0, &termio);
	termio.c_iflag |= (ICRNL | IXON);
	termio.c_oflag |= (CREAD);
	termio.c_lflag |= (ECHO | ECHOE | ECHOK | ICANON | ISIG);
	tcsetattr (0, TCSANOW, &termio);
#endif

	Prog = Basename (argv[0]);
	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

#ifdef	USE_SYSLOG
	OPENLOG ("sulogin");
#endif
	initenv ();
	if (argc > 1) {
		close (0);
		close (1);
		close (2);

		if (open (argv[1], O_RDWR) >= 0) {
			dup (0);
			dup (0);
		} else {
#ifdef	USE_SYSLOG
			SYSLOG (LOG_WARN, "cannot open %s\n", argv[1]);
			closelog ();
#endif
			exit (1);
		}
	}
	if (access (PASSWD_FILE, F_OK) == -1) {	/* must be a password file! */
		(void) puts (_("No password file"));
#ifdef	USE_SYSLOG
		SYSLOG (LOG_WARN, "No password file\n");
		closelog ();
#endif
		exit (1);
	}
#if !defined(DEBUG) && defined(SULOGIN_ONLY_INIT)
	if (getppid () != 1) {	/* parent must be INIT */
#ifdef	USE_SYSLOG
		SYSLOG (LOG_WARN, "Pid == %d, not 1\n", getppid ());
		closelog ();
#endif
		exit (1);
	}
#endif
	if ((isatty (0) == 0) || (isatty (1) == 0) || (isatty (2) == 0)) {
#ifdef	USE_SYSLOG
		closelog ();
#endif
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

	(void) strcpy (name, "root");	/* KLUDGE!!! */

	(void) signal (SIGALRM, catch_signals);	/* exit if the timer expires */
	(void) alarm (ALARM);		/* only wait so long ... */

	while (true) {		/* repeatedly get login/password pairs */
		char *cp;
		pw_entry (name, &pwent);	/* get entry from password file */
		if (pwent.pw_name == (char *) 0) {
			/*
			 * Fail secure
			 */
			(void) puts (_("No password entry for 'root'"));
#ifdef	USE_SYSLOG
			SYSLOG (LOG_WARN, "No password entry for 'root'\n");
			closelog ();
#endif
			exit (1);
		}

		/*
		 * Here we prompt for the root password, or if no password
		 * is given we just exit.
		 */

		/* get a password for root */
		cp = getpass (_(
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
#ifdef	USE_SYSLOG
			SYSLOG (LOG_INFO, "Normal startup\n");
			closelog ();
#endif
			(void) puts ("");
#ifdef	TELINIT
			execl (PATH_TELINIT, "telinit", RUNLEVEL, (char *) 0);
#endif
			exit (0);
		} else {
			STRFCPY (pass, cp);
			strzero (cp);
		}
		if (valid (pass, &pwent)) {	/* check encrypted passwords ... */
			break;	/* ... encrypted passwords matched */
		}

#ifdef	USE_SYSLOG
		SYSLOG (LOG_WARN, "Incorrect root password\n");
#endif
		sleep (2);
		(void) puts (_("Login incorrect"));
	}
	strzero (pass);
	(void) alarm (0);
	(void) signal (SIGALRM, SIG_DFL);
	environ = newenvp;	/* make new environment active */

	(void) puts (_("Entering System Maintenance Mode"));
#ifdef	USE_SYSLOG
	SYSLOG (LOG_INFO, "System Maintenance Mode\n");
#endif

#ifdef	USE_SYSLOG
	closelog ();
#endif
	/* exec the shell finally. */
	err = shell (pwent.pw_shell, (char *) 0, environ);

	return ((err == ENOENT) ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
}

