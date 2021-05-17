/*
 * Copyright (c) 2000 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#ident "$Id$"

#ifndef USE_PAM

#include <stdio.h>
#include <shadow.h>
#include "prototypes.h"
#include "defines.h"
#include "pwauth.h"

void passwd_check (const char *user, const char *passwd, unused const char *progname)
{
	struct spwd *sp;

	sp = getspnam (user); /* !USE_PAM, no need for xgetspnam */
	if (NULL != sp) {
		passwd = sp->sp_pwdp;
	}
	if (pw_auth (passwd, user, PW_LOGIN, (char *) 0) != 0) {
		SYSLOG ((LOG_WARN, "incorrect password for `%s'", user));
		(void) sleep (1);
		fprintf (shadow_logfd, _("Incorrect password for %s.\n"), user);
		exit (EXIT_FAILURE);
	}
}
#else			/* USE_PAM */
extern int errno;	/* warning: ANSI C forbids an empty source file */
#endif			/* USE_PAM */
