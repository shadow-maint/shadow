/*
 * Copyright (c) 1991       , Julianne Frances Haugh
 * Copyright (c) 1991       , Chip Rosenthal
 * Copyright (c) 1996 - 1998, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>
#include "defines.h"
#include <stdio.h>
#include "getdef.h"
#include "prototypes.h"

#ident "$Id$"

/* local function prototypes */
static bool is_listed (const char *cfgin, const char *tty, bool def);

/*
 * This is now rather generic function which decides if "tty" is listed
 * under "cfgin" in config (directly or indirectly). Fallback to default if
 * something is bad.
 */
static bool is_listed (const char *cfgin, const char *tty, bool def)
{
	FILE *fp;
	char buf[200], *cons, *s;

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
		strcpy (buf, cons);
		while ((s = strtok (buf, ":")) != NULL) {
			if (strcmp (s, tty) == 0) {
				return true;
			}

			cons = NULL;
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

	while (fgets (buf, (int) sizeof (buf), fp) != NULL) {
		buf[strlen (buf) - 1] = '\0';
		if (strcmp (buf, tty) == 0) {
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
	return is_listed ("CONSOLE", tty, true);
}

