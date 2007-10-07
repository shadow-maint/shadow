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

#include "rcsid.h"
RCSID("$Id: sgetgrent.c,v 1.4 1998/04/02 21:51:45 marekm Exp $")

#include <stdio.h>
#include <grp.h>
#include "defines.h"

#define	NFIELDS	4

/*
 * list - turn a comma-separated string into an array of (char *)'s
 *
 *	list() converts the comma-separated list of member names into
 *	an array of character pointers.
 *
 *	WARNING: I profiled this once with and without strchr() calls
 *	and found that using a register variable and an explicit loop
 *	works best.  For large /etc/group files, this is a major win.
 *
 * FINALLY added dynamic allocation.  Still need to fix sgetsgent().
 *  --marekm
 */

static char **
list(char *s)
{
	static char **members = 0;
	static int size = 0;  /* max members + 1 */
	int i;
	char **rbuf;

	i = 0;
	for (;;) {
		/* check if there is room for another pointer (to a group
		   member name, or terminating NULL).  */
		if (i >= size) {
			size = i + 100;  /* at least: i + 1 */
			if (members) {
				rbuf = realloc(members, size * sizeof(char *));
			} else {
				/* for old (before ANSI C) implementations of
				   realloc() that don't handle NULL properly */
				rbuf = malloc(size * sizeof(char *));
			}
			if (!rbuf) {
				if (members)
					free(members);
				members = 0;
				size = 0;
				return (char **) 0;
			}
			members = rbuf;
		}
		if (!s || s[0] == '\0')
			break;
		members[i++] = s;
		while (*s && *s != ',')
			s++;
		if (*s)
			*s++ = '\0';
	}
	members[i] = (char *) 0;
	return members;
}


struct group *
sgetgrent(const char *buf)
{
	static char *grpbuf = 0;
	static size_t size = 0;
	static char *grpfields[NFIELDS];
	static struct group grent;
	int	i;
	char	*cp;

	if (strlen(buf) + 1 > size) {
		/* no need to use realloc() here - just free it and
		   allocate a larger block */
		if (grpbuf)
			free(grpbuf);
		size = strlen(buf) + 1000;  /* at least: strlen(buf) + 1 */
		grpbuf = malloc(size);
		if (!grpbuf) {
			size = 0;
			return 0;
		}
	}
	strcpy(grpbuf, buf);

	if ((cp = strrchr(grpbuf, '\n')))
		*cp = '\0';

	for (cp = grpbuf, i = 0; i < NFIELDS && cp; i++) {
		grpfields[i] = cp;
		if ((cp = strchr(cp, ':')))
			*cp++ = 0;
	}
	if (i < (NFIELDS-1) || *grpfields[2] == '\0')
		return 0;
	grent.gr_name = grpfields[0];
	grent.gr_passwd = grpfields[1];
	grent.gr_gid = atoi(grpfields[2]);
	grent.gr_mem = list(grpfields[3]);
	if (!grent.gr_mem)
		return (struct group *) 0;  /* out of memory */

	return &grent;
}
