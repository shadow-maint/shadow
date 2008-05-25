/*
 * Copyright (c) 1997 - 1999, Marek Michałkiewicz
 * Copyright (c) 2001 - 2005, Tomasz Kłoczko
 * Copyright (c) 2008       , Nicolas François
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

#ifdef USE_PAM

#ident "$Id$"


/*
 * Change the user's password using PAM.  Requires libpam and libpam_misc
 * (for misc_conv).  Note: libpam_misc is probably Linux-PAM specific,
 * so you may have to port it if you want to use this code on non-Linux
 * systems with PAM (such as Solaris 2.6).  --marekm
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "defines.h"
#include "pam_defs.h"
#include "prototypes.h"

void do_pam_passwd (const char *user, bool silent, bool change_expired)
{
	pam_handle_t *pamh = NULL;
	int flags = 0, ret;

	if (silent)
		flags |= PAM_SILENT;
	if (change_expired)
		flags |= PAM_CHANGE_EXPIRED_AUTHTOK;

	ret = pam_start ("passwd", user, &conv, &pamh);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr,
			 _("passwd: pam_start() failed, error %d\n"), ret);
		exit (10);	/* XXX */
	}

	ret = pam_chauthtok (pamh, flags);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr, _("passwd: %s\n"), pam_strerror (pamh, ret));
		fputs (_("passwd: password unchanged\n"), stderr);
		pam_end (pamh, ret);
		exit (10);	/* XXX */
	}

	fputs (_("passwd: password updated successfully\n"), stderr);
	(void) pam_end (pamh, PAM_SUCCESS);
}
#else				/* !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */
