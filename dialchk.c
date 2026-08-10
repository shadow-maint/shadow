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
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif
#include "config.h"
#include "dialup.h"

#ifndef	lint
static	char	sccsid[] = "@(#)dialchk.c	3.3	08:54:38	10 Jul 1991";
#endif

extern	char	*pw_encrypt();

/*
 * Check for dialup password
 *
 *	dialcheck tests to see if tty is listed as being a dialup
 *	line.  If so, a dialup password may be required if the shell
 *	is listed as one which requires a second password.
 */

int	dialcheck (tty, shell)
char	*tty;
char	*shell;
{
	char	*crypt ();
	char	*getpass ();
	struct	dialup	*dialup;
	char	*pass;
	char	*cp;

	setduent ();

	if (! isadialup (tty)) {
		endduent ();
		return (1);
	}
	if (! (dialup = getdushell (shell))) {
		endduent ();
		return (1);
	}
	endduent ();

	if (dialup->du_passwd[0] == '\0')
		return (1);

	if (! (pass = getpass ("Dialup Password:")))
		return (0);

	cp = pw_encrypt (pass, dialup->du_passwd);
#if defined(USG)
	memset (pass, 0, strlen (pass));
#else
	bzero (pass, strlen (pass));
#endif
	return (strcmp (cp, dialup->du_passwd) == 0);
}
