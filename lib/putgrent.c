/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
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
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <stdio.h>
#include <grp.h>
#include "prototypes.h"
#include "defines.h"

int
putgrent(const struct group *g, FILE *f)
{
	char *buf, *cp;
	int i;
	size_t size;

	if (!g || !f)
		return -1;

	/* calculate the required buffer size (40 is added for the
	   numeric GID, colons, newline, and terminating NUL).  */
	size = strlen(g->gr_name) + strlen(g->gr_passwd) + 40;
	for (i = 0; g->gr_mem && g->gr_mem[i]; i++)
		size += strlen(g->gr_mem[i]) + 1;

	buf = malloc(size);
	if (!buf)
		return -1;

	sprintf(buf, "%s:%s:%ld:", g->gr_name, g->gr_passwd, (long) g->gr_gid);
	cp = buf + strlen(buf);
	for (i = 0; g->gr_mem && g->gr_mem[i]; i++) {
		if (i > 0)
			*cp++ = ',';
		strcpy(cp, g->gr_mem[i]);
		cp += strlen(cp);
	}
	*cp++ = '\n';
	*cp = '\0';

	if (fputsx(buf, f) == EOF || ferror(f)) {
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}
