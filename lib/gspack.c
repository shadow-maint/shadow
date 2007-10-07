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

#ifdef	SHADOWGRP		/*{ */

#include "rcsid.h"
RCSID ("$Id: gspack.c,v 1.4 2005/03/31 05:14:49 kloczek Exp $")
#include <stdio.h>
#include "defines.h"
/*
 * sgr_pack - convert a shadow group structure to a packed
 *	      shadow group record
 *
 *	sgr_pack takes the shadow group structure and packs
 *	the components in a record.  this record will be
 *	unpacked later by sgr_unpack.
 */
int sgr_pack (const struct sgrp *sgrp, char *buf)
{
	char *cp;
	int i;

	/*
	 * The name and password are both easy - append each string
	 * to the buffer.  These are always the first two strings
	 * in a record.
	 */

	cp = buf;
	strcpy (cp, sgrp->sg_name);
	cp += strlen (cp) + 1;

	strcpy (cp, sgrp->sg_passwd);
	cp += strlen (cp) + 1;

	/*
	 * The arrays of administrators and members are slightly
	 * harder.  Each element is appended as a string, with a
	 * final '\0' appended to serve as a blank string.  The
	 * number of elements is not known in advance, so the
	 * entire collection of administrators must be scanned to
	 * find the start of the members.
	 */

	for (i = 0; sgrp->sg_adm[i]; i++) {
		strcpy (cp, sgrp->sg_adm[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	for (i = 0; sgrp->sg_mem[i]; i++) {
		strcpy (cp, sgrp->sg_mem[i]);
		cp += strlen (cp) + 1;
	}
	*cp++ = '\0';

	return cp - buf;
}

/*
 * sgr_unpack - convert a packed shadow group record to an
 *	        unpacked record
 *
 *	sgr_unpack converts a record which was packed by sgr_pack
 *	into the normal shadow group structure format.
 */

int sgr_unpack (char *buf, int len, struct sgrp *sgrp)
{
	char *org = buf;
	int i;

	/*
	 * The name and password are both easy - they are the first
	 * two strings in the record.
	 */

	sgrp->sg_name = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	sgrp->sg_passwd = buf;
	buf += strlen (buf) + 1;
	if (buf - org > len)
		return -1;

	/*
	 * The administrators and members are slightly more difficult.
	 * The arrays are lists of strings.  Each list is terminated
	 * by a string of length zero.  This string is detected by
	 * looking for an initial character of '\0'.
	 */

	for (i = 0; *buf && i < 1024; i++) {
		sgrp->sg_adm[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	sgrp->sg_adm[i] = (char *) 0;
	if (!*buf)
		buf++;

	for (i = 0; *buf && i < 1024; i++) {
		sgrp->sg_mem[i] = buf;
		buf += strlen (buf) + 1;

		if (buf - org > len)
			return -1;
	}
	sgrp->sg_mem[i] = (char *) 0;

	return 0;
}
#endif				/*} */
