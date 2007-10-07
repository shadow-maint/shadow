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
RCSID ("$Id: pwpack.c,v 1.7 2005/03/31 05:14:49 kloczek Exp $")
#include <sys/types.h>
#include "defines.h"
#include <stdio.h>
#include <pwd.h>
/*
 * pw_pack - convert a (struct pwd) to a packed record
 * WARNING: buf must be large enough, no check for overrun!
 */
int pw_pack (const struct passwd *passwd, char *buf)
{
	char *cp;

	cp = buf;
	strcpy (cp, passwd->pw_name);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_passwd);
	cp += strlen (cp) + 1;

	memcpy (cp, (const char *) &passwd->pw_uid, sizeof passwd->pw_uid);
	cp += sizeof passwd->pw_uid;

	memcpy (cp, (const char *) &passwd->pw_gid, sizeof passwd->pw_gid);
	cp += sizeof passwd->pw_gid;

	strcpy (cp, passwd->pw_gecos);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_dir);
	cp += strlen (cp) + 1;

	strcpy (cp, passwd->pw_shell);
	cp += strlen (cp) + 1;

	return cp - buf;
}

/*
 * pw_unpack - convert a packed (struct pwd) record to a (struct pwd)
 */

int pw_unpack (char *buf, int len, struct passwd *passwd)
{
	char *org = buf;

	memzero (passwd, sizeof *passwd);

	passwd->pw_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	memcpy ((void *) &passwd->pw_uid, (void *) buf, sizeof passwd->pw_uid);
	buf += sizeof passwd->pw_uid;
	if (buf - org > len)
		return -1;

	memcpy ((void *) &passwd->pw_gid, (void *) buf, sizeof passwd->pw_gid);
	buf += sizeof passwd->pw_gid;
	if (buf - org > len)
		return -1;

	passwd->pw_gecos = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_dir = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	passwd->pw_shell = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	return 0;
}
