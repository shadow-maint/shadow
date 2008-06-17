/*
 * Copyright (c) 2005       , Red Hat, Inc.
 * Copyright (c) 2005       , Tomasz Kłoczko
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

/*
 *  Audit helper functions used throughout shadow
 *
 */

#include <config.h>

#ifdef WITH_AUDIT

#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <libaudit.h>
#include <errno.h>
#include <stdio.h>
#include "prototypes.h"
int audit_fd;

void audit_help_open (void)
{
	audit_fd = audit_open ();
	if (audit_fd < 0) {
		/* You get these only when the kernel doesn't have
		 * audit compiled in. */
		if (errno == EINVAL || errno == EPROTONOSUPPORT ||
		    errno == EAFNOSUPPORT)
			return;
		fprintf (stderr,
		         _("Cannot open audit interface - aborting.\n"));
		exit (1);
	}
}

/*
 * This function will log a message to the audit system using a predefined
 * message format. Parameter usage is as follows:
 *
 * type - type of message: AUDIT_USER_CHAUTHTOK for changing any account 
 *	  attributes.
 * pgname - program's name
 * op  -  operation. "adding user", "changing finger info", "deleting group"
 * name - user's account or group name. If not available use NULL.
 * id  -  uid or gid that the operation is being performed on. This is used
 *	  only when user is NULL.
 * result - 1 is "success" and 0 is "failed"
 */
void audit_logger (int type, const char *pgname, const char *op,
		   const char *name, unsigned int id, int result)
{
	if (audit_fd < 0) {
		return;
	} else {
		audit_log_acct_message (audit_fd, type, NULL, op, name, id,
					NULL, NULL, NULL, result);
	}
}

#else				/* WITH_AUDIT */
extern int errno;	/* warning: ANSI C forbids an empty source file */
#endif				/* WITH_AUDIT */

