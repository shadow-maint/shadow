/*
 * Copyright 1989 - 1992, Julianne Frances Haugh
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
RCSID ("$Id: sulog.c,v 1.7 2003/04/22 10:59:22 kloczek Exp $")
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
void
sulog (const char *tty, int success, const char *oldname, const char *name)
{
	char *sulog_file;
	time_t now;
	struct tm *tm;
	FILE *fp;
	mode_t oldmask;

	if ((sulog_file = getdef_str ("SULOG_FILE")) == (char *) 0)
		return;

	oldmask = umask (077);
	fp = fopen (sulog_file, "a+");
	umask (oldmask);
	if (fp == (FILE *) 0)
		return;		/* can't open or create logfile */

	time (&now);
	tm = localtime (&now);

	fprintf (fp, "SU %.02d/%.02d %.02d:%.02d %c %s %s-%s\n",
		 tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		 success ? '+' : '-', tty, oldname, name);

	fflush (fp);
	fclose (fp);
}
