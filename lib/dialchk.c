/*
 * Copyright 1989 - 1991, Julianne Frances Haugh
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
RCSID("$Id: dialchk.c,v 1.6 1999/08/27 19:02:51 marekm Exp $")

#include <stdio.h>
#include "defines.h"
#include "prototypes.h"
#include "dialup.h"
#include "dialchk.h"

/*
 * Check for dialup password
 *
 *	dialcheck tests to see if tty is listed as being a dialup
 *	line.  If so, a dialup password may be required if the shell
 *	is listed as one which requires a second password.
 */

int
dialcheck(const char *tty, const char *sh)
{
	struct	dialup	*dialup;
	char	*pass;
	char	*cp;

	setduent ();

	if (! isadialup (tty)) {
		endduent ();
		return (1);
	}
	if (! (dialup = getdushell (sh))) {
		endduent ();
		return (1);
	}
	endduent ();

	if (dialup->du_passwd[0] == '\0')
		return (1);

	if (! (pass = getpass(_("Dialup Password: "))))
		return (0);

	cp = pw_encrypt (pass, dialup->du_passwd);
	strzero(pass);
	return (strcmp (cp, dialup->du_passwd) == 0);
}
