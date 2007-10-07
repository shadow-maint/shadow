/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
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
RCSID(PKG_VER "$Id: lastlog.c,v 1.6 2000/08/26 18:27:18 marekm Exp $")

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <time.h>

#include "prototypes.h"
#include "defines.h"
#if HAVE_LASTLOG_H
#include <lastlog.h>
#else
#include "lastlog_.h"
#endif

/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif

static FILE *lastlogfile;	/* lastlog file stream */
static off_t user;	/* one single user, specified on command line */
static int days;	/* number of days to consider for print command */
static time_t seconds;		/* that number of days in seconds */

static int uflg = 0;		/* set if user is a valid user id */
static int tflg = 0;		/* print is restricted to most recent days */
static struct lastlog lastlog;	/* scratch structure to play with ... */
static struct stat statbuf;	/* fstat buffer for file size */
static struct passwd *pwent;

extern	char	*optarg;

#define	NOW	(time ((time_t *) 0))

/* local function prototypes */
static void print(void);
static void print_one(const struct passwd *);

int
main(int argc, char **argv)
{
	int	c;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	if ((lastlogfile = fopen (LASTLOG_FILE,"r")) == (FILE *) 0) {
		perror (LASTLOG_FILE);
		exit (1);
	}
	while ((c = getopt (argc, argv, "u:t:")) != EOF) {
		switch (c) {
			case 'u':
				pwent = getpwnam (optarg);
				if (!pwent) {
					fprintf(stderr,
						_("Unknown User: %s\n"),
						optarg);
					exit (1);
				}
				uflg++;
				user = pwent->pw_uid;
				break;
			case 't':
				days = atoi (optarg);
				seconds = days * DAY;
				tflg++;
				break;
		}
	}
	print ();
	fclose (lastlogfile);
	exit (0);
	/*NOTREACHED*/
}

static void
print(void)
{
	off_t	offset;

	if (uflg) {
		offset = (unsigned long) user * sizeof lastlog;
		if (fstat (fileno (lastlogfile), &statbuf)) {
			perror(LASTLOG_FILE);
			return;
		}
		if (offset >= statbuf.st_size)
			return;

		fseek (lastlogfile, offset, SEEK_SET);
		if (fread ((char *) &lastlog, sizeof lastlog, 1,
					lastlogfile) == 1)
			print_one (pwent);
		else
			perror (LASTLOG_FILE);
	} else {
		setpwent ();
		while ((pwent = getpwent ())) {
			user = pwent->pw_uid;
			offset = (unsigned long) user * sizeof lastlog;
			fseek (lastlogfile, offset, SEEK_SET);
			if (fread ((char *) &lastlog, sizeof lastlog, 1,
					lastlogfile) != 1)
				continue;

			if (tflg && NOW - lastlog.ll_time > seconds)
				continue;

			print_one (pwent);
		}
	}
}

static void
print_one(const struct passwd *pw)
{
	static	int	once;
	char	*cp;
	struct	tm	*tm;
#ifdef HAVE_STRFTIME
	char ptime[80];
#endif

	if (! pw)
		return;

	if (! once) {
#ifdef HAVE_LL_HOST
		printf(_("Username         Port     From             Latest\n"));
#else
		printf(_("Username                Port     Latest\n"));
#endif
		once++;
	}
	tm = localtime (&lastlog.ll_time);
#ifdef HAVE_STRFTIME
	strftime(ptime, sizeof(ptime), "%a %b %e %H:%M:%S %z %Y", tm);
	cp = ptime;
#else
	cp = asctime (tm);
	cp[24] = '\0';
#endif

	if(lastlog.ll_time == (time_t) 0)
		cp = _("**Never logged in**\0");

#ifdef HAVE_LL_HOST
	printf ("%-16s %-8.8s %-16.16s %s\n", pw->pw_name,
		lastlog.ll_line, lastlog.ll_host, cp);
#else
	printf ("%-16s\t%-8.8s %s\n", pw->pw_name,
		lastlog.ll_line, cp);
#endif
}
