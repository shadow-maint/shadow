/*
 * Copyright 1989, 1990, 1992, John F. Haugh II
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
#ifndef	BSD
#include <string.h>
#else
#define	strchr	index
#define	strrchr	rindex
#include <strings.h>
#endif

#ifndef	lint
static	char	_sccsid[] = "@(#)env.c	3.1	13:00:03	27 Jul 1992";
#endif

extern	char	**environ;
extern	char	*newenvp[];
extern	int	newenvc;
extern	int	maxenv;

char	*strdup ();
void	free ();

static	char	*forbid[] = {
	"HOME",
	"IFS",
	"PATH",
	"SHELL",
	(char *) 0
};

/*
 * addenv - add a new environmental entry
 */

void
addenv (entry)
char	*entry;
{
	char	*cp;
	int	i;
	int	len;

	if (cp = strchr (entry, '='))
		len = cp - entry;
	else
		return;

	for (i = 0;i < newenvc;i++)
		if (strncmp (entry, newenvp[i], len) == 0 &&
			(newenvp[i][len] == '=' || newenvp[i][len] == '\0'))
			break;

	if (i == maxenv) {
		puts ("Environment overflow");
		return;
	}
	if (i == newenvc) {
		newenvp[newenvc++] = strdup (entry);
	} else {
		free (newenvp[i]);
		newenvp[i] = strdup (entry);
	}
}

/*
 * setenv - copy command line arguments into the environment
 */

void
setenv (argc, argv)
int	argc;
char	**argv;
{
	int	i;
	int	n;
	int	noname = 1;
	char	variable[BUFSIZ];
	char	*cp;

	for (i = 0;i < argc;i++) {
		if ((n = strlen (argv[i])) >= BUFSIZ)
			continue;	/* ignore long entries */

		if (! (cp = strchr (argv[i], '='))) {
			(void) strcpy (variable, argv[i]);
		} else {
			(void) strncpy (variable, argv[i], cp - argv[i]);
			variable[cp - argv[i]] = '\0';
		}
		for (n = 0;forbid[n] != (char *) 0;n++)
			if (strcmp (variable, forbid[n]) == 0)
				break;

		if (forbid[n] != (char *) 0) {
			printf ("You may not change $%s\n", forbid[n]);
			continue;
		}
		if (cp) {
			addenv (argv[i]);
		} else {
			sprintf (variable, "L%d=%s", noname++, argv[i]);
			addenv (variable);
		}
	}
}
