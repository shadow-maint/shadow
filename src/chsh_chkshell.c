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

#include "rcsid.h"
RCSID ("$Id: chsh_chkshell.c,v 1.1 2005/07/07 08:40:27 kloczek Exp $")
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include "prototypes.h"
#include "defines.h"
#ifndef SHELLS_FILE
#define SHELLS_FILE "/etc/shells"
#endif
/*
 * check_shell - see if the user's login shell is listed in /etc/shells
 *
 * The /etc/shells file is read for valid names of login shells.  If the
 * /etc/shells file does not exist the user cannot set any shell unless
 * they are root.
 *
 * If getusershell() is available (Linux, *BSD, possibly others), use it
 * instead of re-implementing it.
 */
int check_shell (const char *sh)
{
	char *cp;
	int found = 0;

#ifndef HAVE_GETUSERSHELL
	char buf[BUFSIZ];
	FILE *fp;
#endif

#ifdef HAVE_GETUSERSHELL
	setusershell ();
	while ((cp = getusershell ())) {
		if (*cp == '#')
			continue;

		if (strcmp (cp, sh) == 0) {
			found = 1;
			break;
		}
	}
	endusershell ();
#else
	if ((fp = fopen (SHELLS_FILE, "r")) == (FILE *) 0)
		return 0;

	while (fgets (buf, sizeof (buf), fp)) {
		if ((cp = strrchr (buf, '\n')))
			*cp = '\0';

		if (buf[0] == '#')
			continue;

		if (strcmp (buf, sh) == 0) {
			found = 1;
			break;
		}
	}
	fclose (fp);
#endif
	return found;
}
