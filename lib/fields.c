/*
 * SPDX-FileCopyrightText: 1990       , Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#include "fields.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "prototypes.h"
#include "string/ctype/strisascii/strisprint.h"
#include "string/ctype/strchrisascii/strchriscntrl.h"
#include "string/strcmp/streq.h"
#include "string/strspn/stpspn.h"
#include "string/strspn/stprspn.h"
#include "string/strtok/stpsep.h"


/*
 * valid_field - insure that a field contains all legal characters
 *
 * Return:
 *	-1	Illegal or control characters are present.
 *	1	Non-ASCII characters are present.
 *	0	All chatacters are legal and ASCII.
 */
int
valid_field_(const char *field, const char *illegal)
{
	if (NULL == field)
		return -1;

	if (strpbrk(field, illegal))
		return -1;
	if (strchriscntrl(field))
		return -1;
	if (strisprint(field))
		return 0;
	if (streq(field, ""))
		return 0;

	return 1;  // !ASCII
}

/*
 * change_field - change a single field if a new value is given.
 *
 * prompt the user with the name of the field being changed and the
 * current value.
 */
void
change_field(char *buf, size_t maxsize, const char *prompt)
{
	char newf[200];
	char *cp;

	if (maxsize > sizeof (newf)) {
		maxsize = sizeof (newf);
	}

	printf ("\t%s [%s]: ", prompt, buf);
	(void) fflush (stdout);
	if (fgets (newf, maxsize, stdin) != newf) {
		return;
	}

	if (stpsep(newf, "\n") == NULL)
		return;

	if (!streq(newf, "")) {
		/*
		 * Remove leading and trailing whitespace.  This also
		 * makes it possible to change the field to empty, by
		 * entering a space.  --marekm
		 */
		stpcpy(stprspn(newf, " \t"), "");
		cp = stpspn(newf, " \t");
		strcpy (buf, cp);
	}
}
