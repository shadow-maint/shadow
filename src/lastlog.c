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
 * ARE DISCLAIMED. IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id: lastlog.c,v 1.26 2006/06/20 20:00:04 kloczek Exp $"

#include <getopt.h>
#include <lastlog.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "defines.h"
#include "prototypes.h"
/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif
/*
 * Global variables
 */
static FILE *lastlogfile;	/* lastlog file stream */
static off_t user;		/* one single user, specified on command line */
static int days;		/* number of days to consider for print command */
static time_t seconds;		/* that number of days in seconds */
static int inverse_days;	/* number of days to consider for print command */
static time_t inverse_seconds;	/* that number of days in seconds */


static int uflg = 0;		/* set if user is a valid user id */
static int tflg = 0;		/* print is restricted to most recent days */
static int bflg = 0;		/* print excludes most recent days */
static struct lastlog lastlog;	/* scratch structure to play with ... */
static struct stat statbuf;	/* fstat buffer for file size */
static struct passwd *pwent;

#define	NOW	(time ((time_t *) 0))

static void usage (void)
{
	fprintf (stdout, _("Usage: lastlog [options]\n"
			   "\n"
			   "Options:\n"
			   "  -b, --before DAYS	print only lastlog records older than DAYS\n"
			   "  -h, --help		display this help message and exit\n"
			   "  -t, --time DAYS	print only lastlog records more recent than DAYS\n"
			   "  -u, --user LOGIN	print lastlog record for user with specified LOGIN\n"
			   "\n"));
	exit (1);
}

static void print_one (const struct passwd *pw)
{
	static int once;
	char *cp;
	struct tm *tm;
	time_t ll_time;

#ifdef HAVE_STRFTIME
	char ptime[80];
#endif

	if (!pw)
		return;

	if (!once) {
#ifdef HAVE_LL_HOST
		printf (_
			("Username         Port     From             Latest\n"));
#else
		printf (_("Username                Port     Latest\n"));
#endif
		once++;
	}
	ll_time = lastlog.ll_time;
	tm = localtime (&ll_time);
#ifdef HAVE_STRFTIME
	strftime (ptime, sizeof (ptime), "%a %b %e %H:%M:%S %z %Y", tm);
	cp = ptime;
#else
	cp = asctime (tm);
	cp[24] = '\0';
#endif

	if (lastlog.ll_time == (time_t) 0)
		cp = _("**Never logged in**\0");

#ifdef HAVE_LL_HOST
	printf ("%-16s %-8.8s %-16.16s %s\n", pw->pw_name,
		lastlog.ll_line, lastlog.ll_host, cp);
#else
	printf ("%-16s\t%-8.8s %s\n", pw->pw_name, lastlog.ll_line, cp);
#endif
}

static void print (void)
{
	off_t offset;

	if (uflg) {
		offset = user * sizeof lastlog;

		if (fstat (fileno (lastlogfile), &statbuf)) {
			perror (LASTLOG_FILE);
			return;
		}
		if (offset >= statbuf.st_size)
			return;

		fseeko (lastlogfile, offset, SEEK_SET);
		if (fread ((char *) &lastlog, sizeof lastlog, 1,
			   lastlogfile) == 1)
			print_one (pwent);
		else
			perror (LASTLOG_FILE);
	} else {
		setpwent ();
		while ((pwent = getpwent ())) {
			user = pwent->pw_uid;
			offset = user * sizeof lastlog;

			fseeko (lastlogfile, offset, SEEK_SET);
			if (fread ((char *) &lastlog, sizeof lastlog, 1,
				   lastlogfile) != 1)
				continue;

			if (tflg && NOW - lastlog.ll_time > seconds)
				continue;

			if (bflg && NOW - lastlog.ll_time < inverse_seconds)
				continue;

			print_one (pwent);
		}
	}
}

int main (int argc, char **argv)
{
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	{
		int c;
		static struct option const longopts[] = {
			{"help", no_argument, NULL, 'h'},
			{"time", required_argument, NULL, 't'},
			{"before", required_argument, NULL, 'b'},
			{"user", required_argument, NULL, 'u'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "ht:b:u:", longopts,
				     NULL)) != -1) {
			switch (c) {
			case 'h':
				usage ();
				break;
			case 't':
				days = atoi (optarg);
				seconds = days * DAY;
				tflg++;
				break;
			case 'b':
				inverse_days = atoi (optarg);
				inverse_seconds = inverse_days * DAY;
				bflg++;
				break;
			case 'u':
				pwent = getpwnam (optarg);
				if (!pwent) {
					fprintf (stderr,
						 _("Unknown User: %s\n"),
						 optarg);
					exit (1);
				}
				uflg++;
				user = pwent->pw_uid;
				break;
			default:
				usage ();
				break;
			}
		}
		if (argc > optind) {
			fprintf (stderr,
			         _("lastlog: unexpected argument: %s\n"),
			         argv[optind]);
			usage();
		}
	}

	if ((lastlogfile = fopen (LASTLOG_FILE, "r")) == (FILE *) 0) {
		perror (LASTLOG_FILE);
		exit (1);
	}

	print ();
	fclose (lastlogfile);
	exit (0);
}
