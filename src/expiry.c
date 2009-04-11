/*
 * Copyright (c) 1994       , Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2001 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
#include "prototypes.h"

/* Global variables */
char *Prog;

/* local function prototypes */
static RETSIGTYPE catch_signals (int);
static void usage (void);

/*
 * catch_signals - signal catcher
 */
static RETSIGTYPE catch_signals (unused int sig)
{
	exit (10);
}

/*
 * usage - print syntax message and exit
 */
static void usage (void)
{
	fputs (_("Usage: expiry {-f|-c}\n"), stderr);
	exit (10);
}

/* 
 * expiry - check and enforce password expiration policy
 *
 *	expiry checks (-c) the current password expiration and forces (-f)
 *	changes when required. It is callable as a normal user command.
 */
int main (int argc, char **argv)
{
	struct passwd *pwd;
	struct spwd *spwd;

	Prog = Basename (argv[0]);

	sanitize_env ();

	/* 
	 * Start by disabling all of the keyboard signals.
	 */
	(void) signal (SIGHUP, catch_signals);
	(void) signal (SIGINT, catch_signals);
	(void) signal (SIGQUIT, catch_signals);
#ifdef	SIGTSTP
	(void) signal (SIGTSTP, catch_signals);
#endif

	/*
	 * expiry takes one of two arguments. The default action is to give
	 * the usage message.
	 */
	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	OPENLOG ("expiry");

	if (   (argc != 2)
	    || (   (strcmp (argv[1], "-f") != 0)
	        && (strcmp (argv[1], "-c") != 0))) {
		usage ();
	}

	/*
	 * Get user entries for /etc/passwd and /etc/shadow
	 */
	pwd = get_my_pwent ();
	if (NULL == pwd) {
		fprintf (stderr, _("%s: Cannot determine your user name.\n"),
		         Prog);
		SYSLOG ((LOG_WARN, "Cannot determine the user name of the caller (UID %lu)",
		         (unsigned long) getuid ()));
		exit (10);
	}
	spwd = getspnam (pwd->pw_name); /* !USE_PAM, No need for xgetspnam */

	/*
	 * If checking accounts, use agecheck() function.
	 */
	if (strcmp (argv[1], "-c") == 0) {

		/*
		 * Print out number of days until expiration.
		 */
		agecheck (spwd);

		/*
		 * Exit with status indicating state of account.
		 */
		exit (isexpired (pwd, spwd));
	}

	/*
	 * If forcing password change, use expire() function.
	 */
	if (strcmp (argv[1], "-f") == 0) {

		/*
		 * Just call expire(). It will force the change or give a
		 * message indicating what to do. And it doesn't return at
		 * all unless the account is unexpired.
		 */
		expire (pwd, spwd);
		exit (0);
	}

	/*
	 * Can't get here ...
	 */
	usage ();
	exit (1);
}

