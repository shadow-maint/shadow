// SPDX-FileCopyrightText: 1991-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1991-1994, Chip Rosenthal
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2007-2010, Nicolas François
// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#ifndef USE_PAM

#include <limits.h>
#include <stdio.h>

#include "io/fgets/fgets.h"
#include "prototypes.h"
#include "string/strtok/stpsep.h"


/*
 * tz - return local timezone name
 *
 * tz() determines the name of the local timezone by reading the
 * contents of the file named by 'path'.
 */
/*@observer@*/
const char *
tz(const char *path)
{
	FILE         *fp;
	static char  buf[LINE_MAX + 1];

	fp = fopen(path, "r");
	if (fp == NULL)
		return "TZ=UTC";

	if (fgets_a(buf, fp) == NULL)
		goto def;
	if (stpsep(buf, "\n") == NULL)
		goto def;

	fclose(fp);
	return buf;
def:
	fclose(fp);
	return "TZ=UTC";
}
#else				/* !USE_PAM */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !USE_PAM */
