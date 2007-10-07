/*
 * Copyright 1993 - 1994, Julianne Frances Haugh
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

#ifndef	HAVE_GETUTENT

#include "defines.h"
#include <stdio.h>
#include <fcntl.h>
#include <utmp.h>

#ifndef	lint
static char rcsid[] = "$Id: utent.c,v 1.5 2005/03/31 05:14:49 kloczek Exp $";
#endif

static int utmp_fd = -1;
static struct utmp utmp_buf;

/*
 * setutent - open or rewind the utmp file
 */

void setutent (void)
{
	if (utmp_fd == -1)
		if ((utmp_fd = open (_UTMP_FILE, O_RDWR)) == -1)
			utmp_fd = open (_UTMP_FILE, O_RDONLY);

	if (utmp_fd != -1)
		lseek (utmp_fd, (off_t) 0L, SEEK_SET);
}

/*
 * endutent - close the utmp file
 */

void endutent (void)
{
	if (utmp_fd != -1)
		close (utmp_fd);

	utmp_fd = -1;
}

/*
 * getutent - get the next record from the utmp file
 */

struct utmp *getutent (void)
{
	if (utmp_fd == -1)
		setutent ();

	if (utmp_fd == -1)
		return 0;

	if (read (utmp_fd, &utmp_buf, sizeof utmp_buf) != sizeof utmp_buf)
		return 0;

	return &utmp_buf;
}

/*
 * getutline - get the utmp entry matching ut_line
 */

struct utmp *getutline (const struct utmp *utent)
{
	struct utmp save;
	struct utmp *new;

	save = *utent;
	while (new = getutent ())
		if (strncmp (new->ut_line, save.ut_line, sizeof new->ut_line))
			continue;
		else
			return new;

	return (struct utmp *) 0;
}
#else
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif
