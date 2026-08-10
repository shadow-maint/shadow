/*
 * Copyright 1990, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifndef	lint
static	char	sccsid[] = "@(#)fields.c	3.2	08:26:23	26 Nov 1990";
#endif

extern	char	*Progname;

/*
 * valid_field - insure that a field contains all legal characters
 *
 * The supplied field is scanned for non-printing and other illegal
 * characters.  If any illegal characters are found, valid_field
 * returns -1.  Zero is returned for success.
 */

int
valid_field (field, illegal)
char	*field;
char	*illegal;
{
	char	*cp;

	for (cp = field;*cp && isprint (*cp) && ! strchr (illegal, *cp);cp++)
		;

	if (*cp)
		return -1;
	else
		return 0;
}

/*
 * change_field - change a single field if a new value is given.
 *
 * prompt the user with the name of the field being changed and the
 * current value.
 */

void
change_field (buf, prompt)
char	*buf;
char	*prompt;
{
	char	new[BUFSIZ];
	char	*cp;

	printf ("\t%s [%s]: ", prompt, buf);
	if (fgets (new, BUFSIZ, stdin) != new)
		return;

	if (cp = strchr (new, '\n'))
		*cp = '\0';
	else
		return;

	if (new[0])
		strcpy (buf, new);
}
