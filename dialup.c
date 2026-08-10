/*
 * Copyright 1989, 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include <stdio.h>
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include "dialup.h"

#ifndef	lint
static	char	sccsid[] = "@(#)dialup.c	3.5	17:31:19	04 Aug 1991";
#endif

static	FILE	*dialpwd;

void
setduent ()
{
	if (dialpwd)
		rewind (dialpwd);
	else
		dialpwd = fopen (DIALPWD, "r");
}

void
endduent ()
{
	if (dialpwd)
		fclose (dialpwd);

	dialpwd = (FILE *) 0;
}

struct dialup *
fgetduent (fp)
FILE	*fp;
{
	static	struct	dialup	dialup;	/* static structure to point to */
	static	char	shell[128];	/* some space for a login shell */
	static	char	passwd[128];	/* some space for dialup password */
	char	buf[BUFSIZ];
	char	*cp;
	char	*cp2;

	if (! fp)
		return 0;

	if (! fp || feof (fp))
		return ((struct dialup *) 0);

	while (fgets (buf, BUFSIZ, fp) == buf && buf[0] == '#')
		;

	if (feof (fp))
		return ((struct dialup *) 0);

	if (cp = strchr (buf, '\n'))
		*cp = '\0';

	if (! (cp = strchr (buf, ':')))
		return ((struct dialup *) 0);

	if (cp - buf > sizeof shell)	/* something is fishy ... */
		return ((struct dialup *) 0);

	*cp++ = '\0';
	(void) strcpy (shell, buf);
	shell[cp - buf] = '\0';

	if (cp2 = strchr (cp, ':'))
		*cp2 = '\0';

	if (strlen (cp) + 1 > sizeof passwd) /* something is REALLY fishy */
		return ((struct dialup *) 0);

	(void) strcpy (passwd, cp);

	dialup.du_shell = shell;
	dialup.du_passwd = passwd;

	return (&dialup);
}

struct dialup *
getduent ()
{
	if (! dialpwd)
		setduent ();

	return fgetduent (dialpwd);
}

struct	dialup	*getdushell (shell)
char	*shell;
{
	struct	dialup	*dialup;

	while (dialup = getduent ()) {
		if (strcmp (shell, dialup->du_shell) == 0)
			return (dialup);

		if (strcmp (dialup->du_shell, "*") == 0)
			return (dialup);
	}
	return ((struct dialup *) 0);
}

int	isadialup (tty)
char	*tty;
{
	FILE	*fp;
	char	buf[BUFSIZ];
	int	dialup = 0;

	if (! (fp = fopen (DIALUPS, "r")))
		return (0);

	while (fgets (buf, BUFSIZ, fp) == buf) {
		if (buf[0] == '#')
			continue;

		buf[strlen (buf) - 1] = '\0';

		if (strcmp (buf, tty) == 0) {
			dialup = 1;
			break;
		}
	}
	fclose (fp);

	return (dialup);
}

int
putduent (dial, fp)
struct	dialup	*dial;
FILE	*fp;
{
	if (! fp || ! dial)
		return -1;

	if (fprintf (fp, "%s:%s\n", dial->du_shell, dial->du_passwd) == EOF)
		return -1;

	return 0;
}
