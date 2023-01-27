/*
 * SPDX-FileCopyrightText: 1992 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 2007 - 2008, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Common code for yes/no prompting
 *
 * Used by pwck.c and grpck.c
 */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include "prototypes.h"

/*
 * yes_or_no - get answer to question from the user
 *
 *	It returns false if no.
 *
 *	If the read_only flag is set, it will print No, and will return
 *	false.
 */
bool yes_or_no (bool read_only)
{
	int c;
	bool result;

	/*
	 * In read-only mode all questions are answered "no".
	 */
	if (read_only) {
		(void) puts (_("No"));
		return false;
	}

	/*
	 * Typically, there's a prompt on stdout, sometimes unflushed.
	 */
	(void) fflush (stdout);

	/*
	 * Get a line and see what the first character is.
	 */
	c = fgetc(stdin);
	/* TODO: use gettext */
	result = (c == 'y' || c == 'Y');

	while (c != '\n' && c != EOF)
		c = fgetc(stdin);

	return result;
}

