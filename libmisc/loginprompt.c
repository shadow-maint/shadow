/*
 * Copyright 1989 - 1993, Julianne Frances Haugh
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

#ident "$Id$"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

static void login_exit (unused int sig)
{
	exit (1);
}

/*
 * login_prompt - prompt the user for their login name
 *
 * login_prompt() displays the standard login prompt.  If ISSUE_FILE
 * is set in login.defs, this file is displayed before the prompt.
 */

void login_prompt (const char *prompt, char *name, int namesize)
{
	char buf[1024];

#define MAX_ENV 32
	char *envp[MAX_ENV];
	int envc;
	char *cp;
	int i;
	FILE *fp;

	RETSIGTYPE (*sigquit) (int);
#ifdef	SIGTSTP
	RETSIGTYPE (*sigtstp) (int);
#endif

	/*
	 * There is a small chance that a QUIT character will be part of
	 * some random noise during a prompt.  Deal with this by exiting
	 * instead of core dumping.  If SIGTSTP is defined, do the same
	 * thing for that signal.
	 */

	sigquit = signal (SIGQUIT, login_exit);
#ifdef	SIGTSTP
	sigtstp = signal (SIGTSTP, login_exit);
#endif

	/*
	 * See if the user has configured the issue file to
	 * be displayed and display it before the prompt.
	 */

	if (prompt) {
		cp = getdef_str ("ISSUE_FILE");
		if (cp && (fp = fopen (cp, "r"))) {
			while ((i = getc (fp)) != EOF)
				putc (i, stdout);

			fclose (fp);
		}
		gethostname (buf, sizeof buf);
		printf (prompt, buf);
		fflush (stdout);
	}

	/* 
	 * Read the user's response.  The trailing newline will be
	 * removed.
	 */

	memzero (buf, sizeof buf);
	if (fgets (buf, sizeof buf, stdin) != buf)
		exit (1);

	cp = strchr (buf, '\n');
	if (!cp)
		exit (1);
	*cp = '\0';		/* remove \n [ must be there ] */

	/*
	 * Skip leading whitespace.  This makes "  username" work right.
	 * Then copy the rest (up to the end or the first "non-graphic"
	 * character into the username.
	 */

	for (cp = buf; *cp == ' ' || *cp == '\t'; cp++);

	for (i = 0; i < namesize - 1 && isgraph (*cp); name[i++] = *cp++);
	while (isgraph (*cp))
		cp++;

	if (*cp)
		cp++;

	name[i] = '\0';

	/*
	 * This is a disaster, at best.  The user may have entered extra
	 * environmental variables at the prompt.  There are several ways
	 * to do this, and I just take the easy way out.
	 */

	if (*cp != '\0') {	/* process new variables */
		char *nvar;
		int count = 1;

		for (envc = 0; envc < MAX_ENV; envc++) {
			nvar = strtok (envc ? (char *) 0 : cp, " \t,");
			if (!nvar)
				break;
			if (strchr (nvar, '=')) {
				envp[envc] = nvar;
			} else {
				envp[envc] = xmalloc (strlen (nvar) + 32);
				sprintf (envp[envc], "L%d=%s", count++, nvar);
			}
		}
		set_env (envc, envp);
	}

	/*
	 * Set the SIGQUIT handler back to its original value
	 */

	signal (SIGQUIT, sigquit);
#ifdef	SIGTSTP
	signal (SIGTSTP, sigtstp);
#endif
}
