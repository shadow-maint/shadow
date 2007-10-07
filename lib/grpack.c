/*
 * Copyright 1990, Julianne Frances Haugh
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
RCSID ("$Id: grpack.c,v 1.4 2005/03/31 05:14:49 kloczek Exp $")
#include <stdio.h>
#include <grp.h>
#include "defines.h"
int gr_pack (const struct group *group, char *buf)
{
	char *cp;
	int i;

	cp = buf;
	strcpy (cp, group->gr_name);
	cp += strlen (cp) + 1;

	strcpy (cp, group->gr_passwd);
	cp += strlen (cp) + 1;

	memcpy (cp, (const char *) &group->gr_gid, sizeof group->gr_gid);
	cp += sizeof group->gr_gid;

	for (i = 0; group->gr_mem[i]; i++) {
		strcpy (cp, group->gr_mem[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	return cp - buf;
}

int gr_unpack (char *buf, int len, struct group *group)
{
	char *org = buf;
	int i;

	group->gr_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	group->gr_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	memcpy ((char *) &group->gr_gid, (char *) buf, sizeof group->gr_gid);
	buf += sizeof group->gr_gid;
	if (buf - org > len)
		return -1;

	for (i = 0; *buf && i < 1024; i++) {
		group->gr_mem[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	group->gr_mem[i] = (char *) 0;
	return 0;
}
