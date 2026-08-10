/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include "config.h"

#ifndef	lint
static	char	sccsid[] = "@(#)login.c	3.4	12:21:53	01 May 1993";
#endif

void	setenv ();

/*
 * login - prompt the user for their login name
 *
 * login() displays the standard login prompt.  If the option
 * ISSUE_FILE_ENAB is set, the file /etc/issue is displayed
 * before the prompt.
 */

void
login (name, prompt)
char	*name;
char	*prompt;
{
	char	buf[BUFSIZ];
	char	*envp[32];
	int	envc;
	char	*cp;
	int	i;
	FILE	*fp;
	SIGTYPE	(*sigquit)();
#ifdef	SIGTSTP
	SIGTYPE	(*sigtstp)();
#endif
	extern	void	exit();

	/*
	 * There is a small chance that a QUIT character will be part of
	 * some random noise during a prompt.  Deal with this by exiting
	 * instead of core dumping.  If SIGTSTP is defined, do the same
	 * thing for that signal.
	 */

	sigquit = signal (SIGQUIT, exit);
#ifdef	SIGTSTP
	sigtstp = signal (SIGTSTP, exit);
#endif

	/*
	 * See if the user has configured the /etc/issue file to
	 * be displayed and display it before the prompt.
	 */

	if (prompt) {
		if (getdef_bool ("ISSUE_FILE_ENAB")) {
			if (fp = fopen ("/etc/issue", "r")) {
				while ((i = getc (fp)) != EOF)
					putc (i, stdout);

				fflush (stdout);
				fclose (fp);
			}
		}
		fputs (prompt, stdout);
	}

	/* 
	 * Read the user's response.  The trailing newline will be
	 * removed.
	 */

#ifndef	BSD
	(void) memset (buf, '\0', sizeof buf);
#else
	bzero (buf, sizeof buf);
#endif
	if (fgets (buf, BUFSIZ, stdin) != buf)
		exit (1);

	buf[strlen (buf) - 1] = '\0';	/* remove \n [ must be there ] */

	/*
	 * Skip leading whitespace.  This makes "  username" work right.
	 * Then copy the rest (up to the end or the first "non-graphic"
	 * character into the username.
	 */

	for (cp = buf;*cp == ' ' || *cp == '\t';cp++)
		;

	for (i = 0;i < BUFSIZ - 1 && isgraph (*cp);name[i++] = *cp++)
		;

	if (*cp)
		cp++;

	name[i] = '\0';

	/*
	 * This is a disaster, at best.  The user may have entered extra
	 * environmental variables at the prompt.  There are several ways
	 * to do this, and I just take the easy way out.
	 */

	if (*cp != '\0') {		/* process new variables */
		static	int	count;
		char	var[BUFSIZ];

		for (envc = 0;envc < 32;envc++) {
			envp[envc] = strtok (envc == 0 ? cp:(char *) 0, " \t,");
			if (envp[envc] == (char *) 0)
				break;

			if (! strchr (envp[envc], '=')) {
				sprintf (var, "L%d=%s", count++, envp[envc]);
				envp[envc] = strdup (var);
			}
		}
		setenv (envc, envp);
	}

	/*
	 * Set the SIGQUIT handler back to its original value
	 */

	(void) signal (SIGQUIT, sigquit);
#ifdef	SIGTSTP
	(void) signal (SIGTSTP, sigtstp);
#endif
}
