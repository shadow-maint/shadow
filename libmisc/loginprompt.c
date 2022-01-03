/*
 * SPDX-FileCopyrightText: 1989 - 1993, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 2000, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008 - 2011, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

static void login_exit (unused int sig)
{
	exit (EXIT_FAILURE);
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
	char *cp;
	int i;
	FILE *fp;

	sighandler_t sigquit;
#ifdef	SIGTSTP
	sighandler_t sigtstp;
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

	if (NULL != prompt) {
		const char *fname = getdef_str ("ISSUE_FILE");
		if (NULL != fname) {
			fp = fopen (fname, "r");
			if (NULL != fp) {
				while ((i = getc (fp)) != EOF) {
					(void) putc (i, stdout);
				}

				(void) fclose (fp);
			}
		}
		(void) gethostname (buf, sizeof buf);
		printf (prompt, buf);
		(void) fflush (stdout);
	}

	/*
	 * Read the user's response.  The trailing newline will be
	 * removed.
	 */

	memzero (buf, sizeof buf);
	if (fgets (buf, (int) sizeof buf, stdin) != buf) {
		exit (EXIT_FAILURE);
	}

	cp = strchr (buf, '\n');
	if (NULL == cp) {
		exit (EXIT_FAILURE);
	}
	*cp = '\0';		/* remove \n [ must be there ] */

	/*
	 * Skip leading whitespace.  This makes "  username" work right.
	 * Then copy the rest (up to the end or the first "non-graphic"
	 * character into the username.
	 */

	for (cp = buf; *cp == ' ' || *cp == '\t'; cp++);

	for (i = 0; i < namesize - 1 && isgraph (*cp); name[i++] = *cp++);
	while (isgraph (*cp)) {
		cp++;
	}

	if ('\0' != *cp) {
		cp++;
	}

	name[i] = '\0';

	/*
	 * This is a disaster, at best.  The user may have entered extra
	 * environmental variables at the prompt.  There are several ways
	 * to do this, and I just take the easy way out.
	 */

	if ('\0' != *cp) {	/* process new variables */
		char *nvar;
		int count = 1;
		int envc;

		for (envc = 0; envc < MAX_ENV; envc++) {
			nvar = strtok ((0 != envc) ? (char *) 0 : cp, " \t,");
			if (NULL == nvar) {
				break;
			}
			if (strchr (nvar, '=') != NULL) {
				envp[envc] = nvar;
			} else {
				size_t len = strlen (nvar) + 32;
				envp[envc] = xmalloc (len);
				(void) snprintf (envp[envc], len,
				                 "L%d=%s", count++, nvar);
			}
		}
		set_env (envc, envp);
	}

	/*
	 * Set the SIGQUIT handler back to its original value
	 */

	(void) signal (SIGQUIT, sigquit);
#ifdef	SIGTSTP
	(void) signal (SIGTSTP, sigtstp);
#endif
}

