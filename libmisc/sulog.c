/*
 * Copyright (c) 1989 - 1992, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
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

#ident "$Id$"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

/*
 * sulog - log a SU command execution result
 */
void sulog (const char *tty, bool success, const char *oldname, const char *name)
{
	const char *sulog_file;
	time_t now;
	struct tm *tm;
	FILE *fp;
	mode_t oldmask;
	gid_t oldgid = 0;

	if (success) {
		SYSLOG ((LOG_INFO,
			"Successful su for %s by %s",name,oldname));
	} else {
		SYSLOG ((LOG_NOTICE,
			"FAILED su for %s by %s",name,oldname));
	}

	sulog_file = getdef_str ("SULOG_FILE");
	if (NULL == sulog_file) {
		return;
	}

	oldgid = getgid ();
	oldmask = umask (077);
	/* Switch to group root to avoid creating the sulog file with
	 * the wrong group ownership. */
	if ((oldgid != 0) && (setgid (0) != 0)) {
		SYSLOG ((LOG_INFO,
		         "su session not logged to %s", sulog_file));
		/* Continue, but do not switch back to oldgid later */
		oldgid = 0;
	}
	fp = fopen (sulog_file, "a+");
	(void) umask (oldmask);
	if ((oldgid != 0) && (setgid (oldgid) != 0)) {
		perror ("setgid");
		SYSLOG ((LOG_ERR,
		         "can't switch back to group `%d' in sulog",
		         oldgid));
		/* Do not return if the group permission were raised. */
		exit (EXIT_FAILURE);
	}
	if (fp == (FILE *) 0) {
		return;		/* can't open or create logfile */
	}

	(void) time (&now);
	tm = localtime (&now);

	fprintf (fp, "SU %.02d/%.02d %.02d:%.02d %c %s %s-%s\n",
		 tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		 success ? '+' : '-', tty, oldname, name);

	(void) fflush (fp);
	fsync (fileno (fp));
	fclose (fp);
	/* TODO: log if failure */
}

