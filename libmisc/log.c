/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
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

#ident "$Id$"

#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include "defines.h"
#include <lastlog.h>
#include "prototypes.h"

/*
 * dolastlog - create lastlog entry
 *
 *	A "last login" entry is created for the user being logged in.  The
 *	UID is extracted from the global (struct passwd) entry and the
 *	TTY information is gotten from the (struct utmp).
 */
void dolastlog (
	struct lastlog *ll,
	const struct passwd *pw,
	/*@unique@*/const char *line,
	/*@unique@*/const char *host)
{
	int fd;
	off_t offset;
	struct lastlog newlog;
	time_t ll_time;

	/*
	 * If the file does not exist, don't create it.
	 */

	fd = open (LASTLOG_FILE, O_RDWR);
	if (-1 == fd) {
		return;
	}

	/*
	 * The file is indexed by UID number.  Seek to the record
	 * for this UID.  Negative UID's will create problems, but ...
	 */

	offset = (off_t) pw->pw_uid * sizeof newlog;

	if (lseek (fd, offset, SEEK_SET) != offset) {
		SYSLOG ((LOG_WARN,
		         "Can't read last lastlog entry for UID %lu in %s. Entry not updated.",
		         (unsigned long) pw->pw_uid, LASTLOG_FILE));
		(void) close (fd);
		return;
	}

	/*
	 * Read the old entry so we can tell the user when they last
	 * logged in.  Then construct the new entry and write it out
	 * the way we read the old one in.
	 */

	if (read (fd, (void *) &newlog, sizeof newlog) != (ssize_t) sizeof newlog) {
		memzero (&newlog, sizeof newlog);
	}
	if (NULL != ll) {
		*ll = newlog;
	}

	ll_time = newlog.ll_time;
	(void) time (&ll_time);
	newlog.ll_time = ll_time;
	strncpy (newlog.ll_line, line, sizeof (newlog.ll_line) - 1);
#if HAVE_LL_HOST
	strncpy (newlog.ll_host, host, sizeof (newlog.ll_host) - 1);
#endif
	if (   (lseek (fd, offset, SEEK_SET) != offset)
	    || (write (fd, (const void *) &newlog, sizeof newlog) != (ssize_t) sizeof newlog)
	    || (close (fd) != 0)) {
		SYSLOG ((LOG_WARN,
		         "Can't write lastlog entry for UID %lu in %s.",
		         (unsigned long) pw->pw_uid, LASTLOG_FILE));
		(void) close (fd);
	}
}

