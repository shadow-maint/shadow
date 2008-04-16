/*
 * Copyright 1989 - 1993, Julianne Frances Haugh
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

#ident "$Id$"

#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "defines.h"
#include "exitcodes.h"
#include "faillog.h"
#include "prototypes.h"
/*
 * Global variables
 */
static FILE *fail;		/* failure file stream */
static uid_t user;		/* one single user, specified on command line */
static int days;		/* number of days to consider for print command */
static time_t seconds;		/* that number of days in seconds */

static int
 aflg = 0,			/* set if all users are to be printed always */
    uflg = 0,			/* set if user is a valid user id */
    tflg = 0;			/* print is restricted to most recent days */

static struct stat statbuf;	/* fstat buffer for file size */

#define	NOW	(time((time_t *) 0))

static void usage (void)
{
	fputs (_("Usage: faillog [options]\n"
	         "\n"
	         "Options:\n"
	         "  -a, --all                     display faillog records for all users\n"
	         "  -h, --help                    display this help message and exit\n"
	         "  -l, --lock-time SEC           after failed login lock accout to SEC seconds\n"
	         "  -m, --maximum MAX             set maximum failed login counters to MAX\n"
	         "  -r, --reset                   reset the counters of login failures\n"
	         "  -t, --time DAYS               display faillog records more recent than DAYS\n"
	         "  -u, --user LOGIN              display faillog record or maintains failure\n"
	         "                                counters and limits (if used with -r, -m or -l\n"
	         "                                options) only for user with LOGIN\n"
	         "\n"), stderr);
	exit (E_USAGE);
}

static void print_one (const struct faillog *fl, uid_t uid)
{
	static int once;
	char *cp;
	struct tm *tm;
	time_t now;
	struct passwd *pwent;

#ifdef HAVE_STRFTIME
	char ptime[80];
#endif

	if (!once) {
		puts (_("Login       Failures Maximum Latest                   On\n"));
		once++;
	}
	pwent = getpwuid (uid); /* local, no need for xgetpwuid */
	time (&now);
	tm = localtime (&fl->fail_time);
#ifdef HAVE_STRFTIME
	strftime (ptime, sizeof (ptime), "%D %H:%M:%S %z", tm);
	cp = ptime;
#endif
	if (pwent) {
		printf ("%-9s   %5d    %5d   ",
			pwent->pw_name, fl->fail_cnt, fl->fail_max);
		if (fl->fail_time) {
			printf ("%s  %s", cp, fl->fail_line);
			if (fl->fail_locktime) {
				if (fl->fail_time + fl->fail_locktime > now
				    && fl->fail_cnt)
					printf (_(" [%lds left]"),
						fl->fail_time +
						fl->fail_locktime - now);
				else
					printf (_(" [%lds lock]"),
						fl->fail_locktime);
			}
		}
		putchar ('\n');
	}
}

static int reset_one (uid_t uid)
{
	off_t offset;
	struct faillog faillog;

	offset = uid * sizeof faillog;
	if (fstat (fileno (fail), &statbuf)) {
		perror (FAILLOG_FILE);
		return 0;
	}
	if (offset >= statbuf.st_size)
		return 0;

	if (fseeko (fail, offset, SEEK_SET) != 0) {
		perror (FAILLOG_FILE);
		return 0;
	}
	if (fread ((char *) &faillog, sizeof faillog, 1, fail) != 1) {
		if (!feof (fail))
			perror (FAILLOG_FILE);

		return 0;
	}
	if (faillog.fail_cnt == 0)
		return 1;	/* don't fill in no holes ... */

	faillog.fail_cnt = 0;

	if (fseeko (fail, offset, SEEK_SET) == 0
	    && fwrite ((char *) &faillog, sizeof faillog, 1, fail) == 1) {
		fflush (fail);
		return 1;
	} else {
		perror (FAILLOG_FILE);
	}
	return 0;
}

static void reset (void)
{
	uid_t uid;

	if (uflg)
		reset_one (user);
	else {
		struct passwd *pwent;

		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			reset_one (pwent->pw_uid);
		}
		endpwent ();
	}
}

static void print (void)
{
	uid_t uid;
	off_t offset;
	struct faillog faillog;

	if (uflg) {
		offset = user * sizeof faillog;
		if (fstat (fileno (fail), &statbuf)) {
			perror (FAILLOG_FILE);
			return;
		}
		if (offset >= statbuf.st_size)
			return;

		fseeko (fail, (off_t) user * sizeof faillog, SEEK_SET);
		if (fread ((char *) &faillog, sizeof faillog, 1, fail) == 1)
			print_one (&faillog, user);
		else
			perror (FAILLOG_FILE);
	} else {
		for (uid = 0;
		     fread ((char *) &faillog, sizeof faillog, 1,
			    fail) == 1; uid++) {

			if (aflg == 0 && faillog.fail_cnt == 0)
				continue;

			if (aflg == 0 && tflg &&
			    NOW - faillog.fail_time > seconds)
				continue;

			if (aflg && faillog.fail_time == 0)
				continue;

			print_one (&faillog, uid);
		}
	}
}

static void setmax_one (uid_t uid, int max)
{
	off_t offset;
	struct faillog faillog;

	offset = uid * sizeof faillog;

	if (fseeko (fail, offset, SEEK_SET) != 0) {
		perror (FAILLOG_FILE);
		return;
	}
	if (fread ((char *) &faillog, sizeof faillog, 1, fail) != 1) {
		if (!feof (fail))
			perror (FAILLOG_FILE);
		memzero (&faillog, sizeof faillog);
	}
	faillog.fail_max = max;

	if (fseeko (fail, offset, SEEK_SET) == 0
	    && fwrite ((char *) &faillog, sizeof faillog, 1, fail) == 1)
		fflush (fail);
	else
		perror (FAILLOG_FILE);
}

static void setmax (int max)
{
	struct passwd *pwent;

	if (uflg) {
		setmax_one (user, max);
	} else {
		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			setmax_one (pwent->pw_uid, max);
		}
		endpwent ();
	}
}

static void set_locktime_one (uid_t uid, long locktime)
{
	off_t offset;
	struct faillog faillog;

	offset = uid * sizeof faillog;

	if (fseeko (fail, offset, SEEK_SET) != 0) {
		perror (FAILLOG_FILE);
		return;
	}
	if (fread ((char *) &faillog, sizeof faillog, 1, fail) != 1) {
		if (!feof (fail))
			perror (FAILLOG_FILE);
		memzero (&faillog, sizeof faillog);
	}
	faillog.fail_locktime = locktime;

	if (fseeko (fail, offset, SEEK_SET) == 0
	    && fwrite ((char *) &faillog, sizeof faillog, 1, fail) == 1)
		fflush (fail);
	else
		perror (FAILLOG_FILE);
}

/*
 * XXX - this needs to be written properly some day, right now it is
 * a quick cut-and-paste hack from the above two functions.  --marekm
 */
static void set_locktime (long locktime)
{
	struct passwd *pwent;

	if (uflg) {
		set_locktime_one (user, locktime);
	} else {
		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			set_locktime_one (pwent->pw_uid, locktime);
		}
		endpwent ();
	}
}

int main (int argc, char **argv)
{
	int anyflag = 0;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/* try to open for read/write, if that fails - read only */
	fail = fopen (FAILLOG_FILE, "r+");
	if (!fail)
		fail = fopen (FAILLOG_FILE, "r");
	if (!fail) {
		perror (FAILLOG_FILE);
		exit (1);
	}

	{
		int option_index = 0;
		int c;
		static struct option long_options[] = {
			{"all", no_argument, NULL, 'a'},
			{"help", no_argument, NULL, 'h'},
			{"lock-secs", required_argument, NULL, 'l'},
			{"maximum", required_argument, NULL, 'm'},
			{"reset", no_argument, NULL, 'r'},
			{"time", required_argument, NULL, 't'},
			{"user", required_argument, NULL, 'u'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c =
			getopt_long (argc, argv, "ahl:m:rt:u:",
				     long_options, &option_index)) != -1) {
			switch (c) {
			case 'a':
				aflg++;
				if (uflg)
					usage ();
				break;
			case 'h':
				usage ();
				break;
			case 'l':
				set_locktime ((long) atoi (optarg));
				anyflag++;
				break;
			case 'm':
				setmax (atoi (optarg));
				anyflag++;
				break;
			case 'r':
				reset ();
				anyflag++;
				break;
			case 't':
				days = atoi (optarg);
				seconds = days * DAY;
				tflg++;
				break;
			case 'u':
			{
				struct passwd *pwent;
				if (aflg)
					usage ();

				/* local, no need for xgetpwnam */
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
			}
			default:
				usage ();
			}
		}
	}

	/* no flags implies -a -p (= print information for all users)  */
	if (!(anyflag || aflg || tflg || uflg))
		aflg++;
	/* (-a or -t days or -u user) and no other flags implies -p
	   (= print information for selected users) */
	if (!anyflag && (aflg || tflg || uflg))
		print ();
	fclose (fail);

	exit (E_SUCCESS);
}
