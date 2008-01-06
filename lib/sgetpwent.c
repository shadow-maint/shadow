/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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

#ident "$Id$"

#include <sys/types.h>
#include "defines.h"
#include <stdio.h>
#include <pwd.h>
#include "prototypes.h"

#define	NFIELDS	7

/*
 * sgetpwent - convert a string to a (struct passwd)
 *
 * sgetpwent() parses a string into the parts required for a password
 * structure.  Strict checking is made for the UID and GID fields and
 * presence of the correct number of colons.  Any failing tests result
 * in a NULL pointer being returned.
 *
 * NOTE: This function uses hard-coded string scanning functions for
 *	performance reasons.  I am going to come up with some conditional
 *	compilation glarp to improve on this in the future.
 */
struct passwd *sgetpwent (const char *buf)
{
	static struct passwd pwent;
	static char pwdbuf[1024];
	register int i;
	register char *cp;
	char *ep;
	char *fields[NFIELDS];

	/*
	 * Copy the string to a static buffer so the pointers into
	 * the password structure remain valid.
	 */

	if (strlen (buf) >= sizeof pwdbuf)
		return 0;	/* fail if too long */
	strcpy (pwdbuf, buf);

	/*
	 * Save a pointer to the start of each colon separated
	 * field.  The fields are converted into NUL terminated strings.
	 */

	for (cp = pwdbuf, i = 0; i < NFIELDS && cp; i++) {
		fields[i] = cp;
		while (*cp && *cp != ':')
			++cp;

		if (*cp)
			*cp++ = '\0';
		else
			cp = 0;
	}

	/*
	 * There must be exactly NFIELDS colon separated fields or
	 * the entry is invalid.  Also, the UID and GID must be non-blank.
	 */

	if (i != NFIELDS || *fields[2] == '\0' || *fields[3] == '\0')
		return 0;

	/*
	 * Each of the fields is converted the appropriate data type
	 * and the result assigned to the password structure.  If the
	 * UID or GID does not convert to an integer value, a NULL
	 * pointer is returned.
	 */

	pwent.pw_name = fields[0];
	pwent.pw_passwd = fields[1];
	if (fields[2][0] == '\0' ||
	    ((pwent.pw_uid = strtol (fields[2], &ep, 10)) == 0 && *ep)) {
		return 0;
	}
	if (fields[3][0] == '\0' ||
	    ((pwent.pw_gid = strtol (fields[3], &ep, 10)) == 0 && *ep)) {
		return 0;
	}
	pwent.pw_gecos = fields[4];
	pwent.pw_dir = fields[5];
	pwent.pw_shell = fields[6];

	return &pwent;
}
