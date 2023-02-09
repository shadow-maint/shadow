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
	char buf[80];

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
	/* TODO: use gettext */
	if (fgets (buf, sizeof buf, stdin) == buf) {
		return buf[0] == 'y' || buf[0] == 'Y';
	}

	return false;
}

