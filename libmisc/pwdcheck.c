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

#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#ifdef USE_PAM
#include "pam_defs.h"
#else
#include <shadow.h>
#include "pwauth.h"
#endif
#define WRONGPWD2	"incorrect password for `%s'"
void passwd_check (const char *user, const char *passwd, const char *progname)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retcode;

	if (pam_start (progname, user, &conv, &pamh)) {
	      bailout:
		SYSLOG ((LOG_WARN, WRONGPWD2, user));
		sleep (1);
		fprintf (stderr, _("Incorrect password for %s.\n"), user);
		exit (1);
	}
	if (pam_authenticate (pamh, 0))
		goto bailout;

	retcode = pam_acct_mgmt (pamh, 0);
	if (retcode == PAM_NEW_AUTHTOK_REQD)
		retcode = pam_chauthtok (pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
	if (retcode)
		goto bailout;

	if (pam_setcred (pamh, 0))
		goto bailout;

	/* no need to establish a session; this isn't a session-oriented
	 * activity... */

#else				/* !USE_PAM */

	struct spwd *sp;

	if ((sp = getspnam (user))) /* !USE_PAM, no need for xgetspnam */
		passwd = sp->sp_pwdp;
	if (pw_auth (passwd, user, PW_LOGIN, (char *) 0) != 0) {
		SYSLOG ((LOG_WARN, WRONGPWD2, user));
		sleep (1);
		fprintf (stderr, _("Incorrect password for %s.\n"), user);
		exit (1);
	}
#endif				/* !USE_PAM */
}
