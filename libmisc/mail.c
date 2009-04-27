/*
 * Copyright (c) 1989 - 1991, Julianne Frances Haugh
 * Copyright (c) 1996 - 1998, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
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
#include "prototypes.h"
#include "defines.h"
#include <assert.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include "getdef.h"

#ident "$Id$"


void mailcheck (void)
{
	struct stat statbuf;
	char *mailbox;

	if (!getdef_bool ("MAIL_CHECK_ENAB")) {
		return;
	}

	/*
	 * Check incoming mail in Maildir format - J.
	 */
	mailbox = getenv ("MAILDIR");
	if (NULL != mailbox) {
		char *newmail;
		size_t len = strlen (mailbox) + 5;
		int wlen;

		newmail = xmalloc (len);
		wlen = snprintf (newmail, len, "%s/new", mailbox);
		assert (wlen == (int) len - 1);

		if (stat (newmail, &statbuf) != -1 && statbuf.st_size != 0) {
			if (statbuf.st_mtime > statbuf.st_atime) {
				free (newmail);
				(void) puts (_("You have new mail."));
				return;
			}
		}
		free (newmail);
	}

	mailbox = getenv ("MAIL");
	if (NULL == mailbox) {
		return;
	}

	if (   (stat (mailbox, &statbuf) == -1)
	    || (statbuf.st_size == 0)) {
		(void) puts (_("No mail."));
	} else if (statbuf.st_atime > statbuf.st_mtime) {
		(void) puts (_("You have mail."));
	} else {
		(void) puts (_("You have new mail."));
	}
}

