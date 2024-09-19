/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1991 - 1994, Chip Rosenthal
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2010, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ifndef USE_PAM

#ident "$Id$"

#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "getdef.h"
#include "prototypes.h"
#include "string/strtok/stpsep.h"


/*
 * tz - return local timezone name
 *
 * tz() determines the name of the local timezone by reading the
 * contents of the file named by ``fname''.
 */
/*@observer@*/const char *tz (const char *fname)
{
	FILE *fp = NULL;
	const char *result;
	static char tzbuf[BUFSIZ];

	fp = fopen (fname, "r");
	if (   (NULL == fp)
	    || (fgets (tzbuf, sizeof (tzbuf), fp) == NULL)) {
		result = "TZ=CST6CDT";
	} else {
		stpsep(tzbuf, "\n");
		result = tzbuf;
	}

	if (NULL != fp) {
		(void) fclose (fp);
	}

	return result;
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */

