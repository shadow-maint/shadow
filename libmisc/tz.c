/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1991 - 1994, Chip Rosenthal
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

#ident "$Id$"

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "prototypes.h"
#include "getdef.h"
/*
 * tz - return local timezone name
 *
 * tz() determines the name of the local timezone by reading the
 * contents of the file named by ``fname''.
 */
char *tz (const char *fname)
{
	FILE *fp = 0;
	static char tzbuf[BUFSIZ];
	const char *def_tz = "TZ=CST6CDT";

	if ((fp = fopen (fname, "r")) == NULL ||
	    fgets (tzbuf, sizeof (tzbuf), fp) == NULL) {
#ifndef USE_PAM
		if (!(def_tz = getdef_str ("ENV_TZ")) || def_tz[0] == '/')
			def_tz = "TZ=CST6CDT";
#endif				/* !USE_PAM */

		strcpy (tzbuf, def_tz);
	} else
		tzbuf[strlen (tzbuf) - 1] = '\0';

	if (fp)
		(void) fclose (fp);

	return tzbuf;
}
