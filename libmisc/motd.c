/*
 * Copyright (c) 1989 - 1991, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2010       , Nicolas François
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
 * motd -- output the /etc/motd file
 *
 * motd() determines the name of a login announcement file and outputs
 * it to the user's terminal at login time.  The MOTD_FILE configuration
 * option is a colon-delimited list of filenames.
 */
void motd (void)
{
	FILE *fp;
	char *motdlist;
	const char *motdfile;
	char *mb;
	register int c;

	motdfile = getdef_str ("MOTD_FILE");
	if (NULL == motdfile) {
		return;
	}

	motdlist = xstrdup (motdfile);

	for (mb = motdlist; ;mb = NULL) {
		motdfile = strtok (mb, ":");
		if (NULL == motdfile) {
			break;
		}

		fp = fopen (motdfile, "r");
		if (NULL != fp) {
			while ((c = getc (fp)) != EOF) {
				putchar (c);
			}
			fclose (fp);
		}
	}
	fflush (stdout);

	free (motdlist);
}

