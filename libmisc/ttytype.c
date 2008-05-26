/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2008       , Nicolas François
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
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"
/*
 * ttytype - set ttytype from port to terminal type mapping database
 */
void ttytype (const char *line)
{
	FILE *fp;
	char buf[BUFSIZ];
	char *typefile;
	char *cp;
	char type[BUFSIZ];
	char port[BUFSIZ];

	if (getenv ("TERM"))
		return;
	if ((typefile = getdef_str ("TTYTYPE_FILE")) == NULL)
		return;
	if (access (typefile, F_OK) != 0)
		return;

	fp = fopen (typefile, "r");
	if (NULL == fp) {
		perror (typefile);
		return;
	}
	while (fgets (buf, sizeof buf, fp) == buf) {
		if (buf[0] == '#') {
			continue;
		}

		cp = strchr (buf, '\n');
		if (NULL != cp) {
			*cp = '\0';
		}

		if ((sscanf (buf, "%s %s", type, port) == 2) &&
		    (strcmp (line, port) == 0)) {
			break;
		}
	}
	if ((feof (fp) == 0) && (ferror (fp) == 0)) {
		addenv ("TERM", type);
	}

	(void) fclose (fp);
}

