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
#ifdef	SHADOWPWD		/*{ */

#include "rcsid.h"
RCSID ("$Id: sppack.c,v 1.4 2005/03/31 05:14:49 kloczek Exp $")
#include <stdio.h>
#include <sys/types.h>
#include "defines.h"
int spw_pack (const struct spwd *spwd, char *buf)
{
	char *cp;

	cp = buf;
	strcpy (cp, spwd->sp_namp);
	cp += strlen (cp) + 1;

	strcpy (cp, spwd->sp_pwdp);
	cp += strlen (cp) + 1;

	memcpy (cp, &spwd->sp_min, sizeof spwd->sp_min);
	cp += sizeof spwd->sp_min;

	memcpy (cp, &spwd->sp_max, sizeof spwd->sp_max);
	cp += sizeof spwd->sp_max;

	memcpy (cp, &spwd->sp_lstchg, sizeof spwd->sp_lstchg);
	cp += sizeof spwd->sp_lstchg;

	memcpy (cp, &spwd->sp_warn, sizeof spwd->sp_warn);
	cp += sizeof spwd->sp_warn;

	memcpy (cp, &spwd->sp_inact, sizeof spwd->sp_inact);
	cp += sizeof spwd->sp_inact;

	memcpy (cp, &spwd->sp_expire, sizeof spwd->sp_expire);
	cp += sizeof spwd->sp_expire;

	memcpy (cp, &spwd->sp_flag, sizeof spwd->sp_flag);
	cp += sizeof spwd->sp_flag;

	return cp - buf;
}

int spw_unpack (char *buf, int len, struct spwd *spwd)
{
	char *org = buf;

	spwd->sp_namp = buf;
	buf += strlen (buf) + 1;

	spwd->sp_pwdp = buf;
	buf += strlen (buf) + 1;

	memcpy (&spwd->sp_min, buf, sizeof spwd->sp_min);
	buf += sizeof spwd->sp_min;

	memcpy (&spwd->sp_max, buf, sizeof spwd->sp_max);
	buf += sizeof spwd->sp_max;

	memcpy (&spwd->sp_lstchg, buf, sizeof spwd->sp_lstchg);
	buf += sizeof spwd->sp_lstchg;

	memcpy (&spwd->sp_warn, buf, sizeof spwd->sp_warn);
	buf += sizeof spwd->sp_warn;

	memcpy (&spwd->sp_inact, buf, sizeof spwd->sp_inact);
	buf += sizeof spwd->sp_inact;

	memcpy (&spwd->sp_expire, buf, sizeof spwd->sp_expire);
	buf += sizeof spwd->sp_expire;

	memcpy (&spwd->sp_flag, buf, sizeof spwd->sp_flag);
	buf += sizeof spwd->sp_flag;

	if (buf - org > len)
		return -1;

	return 0;
}
#endif				/*} */
