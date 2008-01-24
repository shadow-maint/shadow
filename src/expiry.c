/*
 * Copyright 1994, Julianne Frances Haugh
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

#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
#include "prototypes.h"
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
	char *Prog = argv[0];

	sanitize_env ();

	/* 
	 * Start by disabling all of the keyboard signals.
	 */
	signal (SIGHUP, catch_signals);
	signal (SIGINT, catch_signals);
	signal (SIGQUIT, catch_signals);
#ifdef	SIGTSTP
	signal (SIGTSTP, catch_signals);
#endif

	/*
	 * expiry takes one of two arguments. The default action is to give
	 * the usage message.
	 */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (argc != 2 || (strcmp (argv[1], "-f") && strcmp (argv[1], "-c")))
		usage ();

	/*
	 * Get user entries for /etc/passwd and /etc/shadow
	 */
	if (!(pwd = get_my_pwent ())) {
		fprintf (stderr, _("%s: unknown user\n"), Prog);
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
		agecheck (pwd, spwd);

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
