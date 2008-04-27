/*
 * Copyright (c) 1990 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1999, Marek Michałkiewicz
 * Copyright (c) 2005       , Tomasz Kłoczko
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

#include <stdio.h>
#include "defines.h"
#include "prototypes.h"

#ident "$Id$"


char *fgetsx (char *buf, int cnt, FILE * f)
{
	char *cp = buf;
	char *ep;

	while (cnt > 0) {
		if (fgets (cp, cnt, f) == 0) {
			if (cp == buf)
				return 0;
			else
				break;
		}
		if ((ep = strrchr (cp, '\\')) && *(ep + 1) == '\n') {
			if ((cnt -= ep - cp) > 0)
				*(cp = ep) = '\0';
		} else
			break;
	}
	return buf;
}

int fputsx (const char *s, FILE * stream)
{
	int i;

	for (i = 0; *s; i++, s++) {
		if (putc (*s, stream) == EOF)
			return EOF;

#if 0				/* The standard getgr*() can't handle that.  --marekm */
		if (i > (BUFSIZ / 2)) {
			if (putc ('\\', stream) == EOF ||
			    putc ('\n', stream) == EOF)
				return EOF;

			i = 0;
		}
#endif
	}
	return 0;
}
