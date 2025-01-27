/*
 * SPDX-FileCopyrightText: 1991       , Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1991       , Chip Rosenthal
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2007 - 2010, Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "getdef.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strcpy/strtcpy.h"
#include "string/strtok/stpsep.h"


/*
 * This is now rather generic function which decides if "tty" is listed
 * under "cfgin" in config (directly or indirectly). Fallback to default if
 * something is bad.
 */
static bool
is_listed(const char *cfgin, const char *tty, bool def)
{
	FILE *fp;
	char buf[1024], *s;
	const char *cons;

	/*
	 * If the CONSOLE configuration definition isn't given,
	 * fallback to default.
	 */

	cons = getdef_str (cfgin);
	if (NULL == cons) {
		return def;
	}

	/*
	 * If this isn't a filename, then it is a ":" delimited list of
	 * console devices upon which root logins are allowed.
	 */

	if (*cons != '/') {
		char *pbuf;

		STRTCPY(buf, cons);
		pbuf = buf;
		while (NULL != (s = strsep(&pbuf, ":"))) {
			if (streq(s, tty)) {
				return true;
			}
		}
		return false;
	}

	/*
	 * If we can't open the console list, then call everything a
	 * console - otherwise root will never be allowed to login.
	 */

	fp = fopen (cons, "r");
	if (NULL == fp) {
		return def;
	}

	/*
	 * See if this tty is listed in the console file.
	 */

	while (fgets (buf, sizeof (buf), fp) != NULL) {
		stpsep(buf, "\n");
		if (streq(buf, tty)) {
			(void) fclose (fp);
			return true;
		}
	}

	/*
	 * This tty isn't a console.
	 */

	(void) fclose (fp);
	return false;
}

/*
 * console - return 1 if the "tty" is a console device, else 0.
 *
 * Note - we need to take extreme care here to avoid locking out root logins
 * if something goes awry.  That's why we do things like call everything a
 * console if the consoles file can't be opened.  Because of this, we must
 * warn the user to protect against the remove of the consoles file since
 * that would allow an unauthorized root login.
 */

bool console (const char *tty)
{
	tty = strprefix(tty, "/dev/") ?: tty;

	return is_listed ("CONSOLE", tty, true);
}

