/*
 * Copyright (c) 1989 - 1993, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2002 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2010, Nicolas François
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

#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include "defines.h"
#include "faillog.h"
#include "prototypes.h"
/*@-exitarg@*/
#include "exitcodes.h"

/* local function prototypes */
static void usage (int status);
static void print_one (/*@null@*/const struct passwd *pw, bool force);
static void set_locktime (long locktime);
static bool set_locktime_one (uid_t uid, long locktime);
static void setmax (int max);
static bool setmax_one (uid_t uid, int max);
static void print (void);
static bool reset_one (uid_t uid);
static void reset (void);

/*
 * Global variables
 */
static FILE *fail;		/* failure file stream */
static time_t seconds;		/* that number of days in seconds */
static unsigned long umin;	/* if uflg and has_umin, only display users with uid >= umin */
static bool has_umin = false;
static unsigned long umax;	/* if uflg and has_umax, only display users with uid <= umax */
static bool has_umax = false;
static bool errors = false;

static bool aflg = false;	/* set if all users are to be printed always */
static bool uflg = false;	/* set if user is a valid user id */
static bool tflg = false;	/* print is restricted to most recent days */
static bool lflg = false;	/* set the locktime */
static bool mflg = false;	/* set maximum failed login counters */
static bool rflg = false;	/* reset the counters of login failures */

static struct stat statbuf;	/* fstat buffer for file size */

#define	NOW	(time((time_t *) 0))

static void usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options]\n"
	                  "\n"
	                  "Options:\n"),
	                "faillog");
	(void) fputs (_("  -a, --all                     display faillog records for all users\n"), usageout);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -l, --lock-secs SEC           after failed login lock account for SEC seconds\n"), usageout);
	(void) fputs (_("  -m, --maximum MAX             set maximum failed login counters to MAX\n"), usageout);
	(void) fputs (_("  -r, --reset                   reset the counters of login failures\n"), usageout);
	(void) fputs (_("  -t, --time DAYS               display faillog records more recent than DAYS\n"), usageout);
	(void) fputs (_("  -u, --user LOGIN/RANGE        display faillog record or maintains failure\n"
	                "                                counters and limits (if used with -r, -m,\n"
	                "                                or -l) only for the specified LOGIN(s)\n"), usageout);
	(void) fputs ("\n", usageout);
	exit (status);
}

static void print_one (/*@null@*/const struct passwd *pw, bool force)
{
	static bool once = false;
	struct tm *tm;
	off_t offset;
	struct faillog fl;
	time_t now;

#ifdef HAVE_STRFTIME
	char *cp;
	char ptime[80];
#endif

	if (NULL == pw) {
		return;
	}

	offset = pw->pw_uid * sizeof (fl);
	if (offset <= (statbuf.st_size - sizeof (fl))) {
		/* fseeko errors are not really relevant for us. */
		int err = fseeko (fail, offset, SEEK_SET);
		assert (0 == err);
		/* faillog is a sparse file. Even if no entries were
		 * entered for this user, which should be able to get the
		 * empty entry in this case.
		 */
		if (fread ((char *) &fl, sizeof (fl), 1, fail) != 1) {
			fprintf (stderr,
			         _("faillog: Failed to get the entry for UID %lu\n"),
			         (unsigned long int)pw->pw_uid);
			return;
		}
	} else {
		/* Outsize of the faillog file.
		 * Behave as if there were a missing entry (same behavior
		 * as if we were reading an non existing entry in the
		 * sparse faillog file).
		 */
		memzero (&fl, sizeof (fl));
	}

	/* Nothing to report */
	if (!force && (0 == fl.fail_time)) {
		return;
	}

	(void) time(&now);

	/* Filter out entries that do not match with the -t option */
	if (tflg && ((now - fl.fail_time) > seconds)) {
		return;
	}

	/* Print the header only once */
	if (!once) {
		puts (_("Login       Failures Maximum Latest                   On\n"));
		once = true;
	}

	tm = localtime (&fl.fail_time);
#ifdef HAVE_STRFTIME
	strftime (ptime, sizeof (ptime), "%D %H:%M:%S %z", tm);
	cp = ptime;
#endif
	printf ("%-9s   %5d    %5d   ",
	        pw->pw_name, fl.fail_cnt, fl.fail_max);
	/* FIXME: cp is not defined ifndef HAVE_STRFTIME */
	printf ("%s  %s", cp, fl.fail_line);
	if (0 != fl.fail_locktime) {
		if (   ((fl.fail_time + fl.fail_locktime) > now)
		    && (0 != fl.fail_cnt)) {
			printf (_(" [%lus left]"),
			        (unsigned long) fl.fail_time + fl.fail_locktime - now);
		} else {
			printf (_(" [%lds lock]"),
			        fl.fail_locktime);
		}
	}
	putchar ('\n');
}

static void print (void)
{
	if (uflg && has_umin && has_umax && (umin==umax)) {
		print_one (getpwuid ((uid_t)umin), true);
	} else {
		/* We only print records for existing users.
		 * Loop based on the user database instead of reading the
		 * whole file. We will have to query the database anyway
		 * so except for very small ranges and large user
		 * database, this should not be a performance issue.
		 */
		struct passwd *pwent;

		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			if (   uflg
			    && (   (has_umin && (pwent->pw_uid < (uid_t)umin))
			        || (has_umax && (pwent->pw_uid > (uid_t)umax)))) {
				continue;
			}
			print_one (pwent, aflg);
		}
		endpwent ();
	}
}

/*
 * reset_one - Reset the fail count for one user
 *
 * This returns a boolean indicating if an error occurred.
 */
static bool reset_one (uid_t uid)
{
	off_t offset;
	struct faillog fl;

	offset = uid * sizeof (fl);
	if (offset <= (statbuf.st_size - sizeof (fl))) {
		/* fseeko errors are not really relevant for us. */
		int err = fseeko (fail, offset, SEEK_SET);
		assert (0 == err);
		/* faillog is a sparse file. Even if no entries were
		 * entered for this user, which should be able to get the
		 * empty entry in this case.
		 */
		if (fread ((char *) &fl, sizeof (fl), 1, fail) != 1) {
			fprintf (stderr,
			         _("faillog: Failed to get the entry for UID %lu\n"),
			         (unsigned long int)uid);
			return true;
		}
	} else {
		/* Outsize of the faillog file.
		 * Behave as if there were a missing entry (same behavior
		 * as if we were reading an non existing entry in the
		 * sparse faillog file).
		 */
		memzero (&fl, sizeof (fl));
	}

	if (0 == fl.fail_cnt) {
		/* If the count is already null, do not write in the file.
		 * This avoids writing 0 when no entries were present for
		 * the user.
		 */
		return false;
	}

	fl.fail_cnt = 0;

	if (   (fseeko (fail, offset, SEEK_SET) == 0)
	    && (fwrite ((char *) &fl, sizeof (fl), 1, fail) == 1)) {
		(void) fflush (fail);
		return false;
	}

	fprintf (stderr,
	         _("faillog: Failed to reset fail count for UID %lu\n"),
	         (unsigned long int)uid);
	return true;
}

static void reset (void)
{
	if (uflg && has_umin && has_umax && (umin==umax)) {
		if (reset_one ((uid_t)umin)) {
			errors = true;
		}
	} else {
		/* There is no need to reset outside of the faillog
		 * database.
		 */
		uid_t uidmax = statbuf.st_size / sizeof (struct faillog);
		if (uidmax > 1) {
			uidmax--;
		}
		if (has_umax && (uid_t)umax < uidmax) {
			uidmax = (uid_t)umax;
		}

		/* Reset all entries in the specified range.
		 * Non existing entries will not be touched.
		 */
		if (aflg) {
			/* Entries for non existing users are also reset.
			 */
		uid_t uid = 0;

		/* Make sure we stay in the umin-umax range if specified */
		if (has_umin) {
			uid = (uid_t)umin;
		}

		while (uid <= uidmax) {
			if (reset_one (uid)) {
				errors = true;
			}
			uid++;
		}
		} else {
			/* Only reset records for existing users.
			 */
			struct passwd *pwent;

			setpwent ();
			while ( (pwent = getpwent ()) != NULL ) {
				if (   uflg
				    && (   (has_umin && (pwent->pw_uid < (uid_t)umin))
				        || (pwent->pw_uid > (uid_t)uidmax))) {
					continue;
				}
				if (reset_one (pwent->pw_uid)) {
					errors = true;
				}
			}
			endpwent ();
		}
	}
}

/*
 * setmax_one - Set the maximum failed login counter for one user
 *
 * This returns a boolean indicating if an error occurred.
 */
static bool setmax_one (uid_t uid, int max)
{
	off_t offset;
	struct faillog fl;

	offset = (off_t) uid * sizeof (fl);
	if (offset <= (statbuf.st_size - sizeof (fl))) {
		/* fseeko errors are not really relevant for us. */
		int err = fseeko (fail, offset, SEEK_SET);
		assert (0 == err);
		/* faillog is a sparse file. Even if no entries were
		 * entered for this user, which should be able to get the
		 * empty entry in this case.
		 */
		if (fread ((char *) &fl, sizeof (fl), 1, fail) != 1) {
			fprintf (stderr,
			         _("faillog: Failed to get the entry for UID %lu\n"),
			         (unsigned long int)uid);
			return true;
		}
	} else {
		/* Outsize of the faillog file.
		 * Behave as if there were a missing entry (same behavior
		 * as if we were reading an non existing entry in the
		 * sparse faillog file).
		 */
		memzero (&fl, sizeof (fl));
	}

	if (max == fl.fail_max) {
		/* If the max is already set to the right value, do not
		 * write in the file.
		 * This avoids writing 0 when no entries were present for
		 * the user and the max argument is 0.
		 */
		return false;
	}

	fl.fail_max = max;

	if (   (fseeko (fail, offset, SEEK_SET) == 0)
	    && (fwrite ((char *) &fl, sizeof (fl), 1, fail) == 1)) {
		(void) fflush (fail);
		return false;
	}

	fprintf (stderr,
	         _("faillog: Failed to set max for UID %lu\n"),
	         (unsigned long int)uid);
	return true;
}

static void setmax (int max)
{
	if (uflg && has_umin && has_umax && (umin==umax)) {
		if (setmax_one ((uid_t)umin, max)) {
			errors = true;
		}
	} else {
		/* Set max for entries in the specified range.
		 * If max is unchanged for an entry, the entry is not touched.
		 * If max is null, and no entries exist for this user, no
		 * entries will be created.
		 */
		if (aflg) {
		/* Entries for non existing user are also taken into
		 * account (in order to define policy for future users).
		 */
		uid_t uid = 0;
		/* The default umax value is based on the size of the
		 * faillog database.
		 */
		uid_t uidmax = statbuf.st_size / sizeof (struct faillog);
		if (uidmax > 1) {
			uidmax--;
		}

		/* Make sure we stay in the umin-umax range if specified */
		if (has_umin) {
			uid = (uid_t)umin;
		}
		if (has_umax) {
			uidmax = (uid_t)umax;
		}

		while (uid <= uidmax) {
			if (setmax_one (uid, max)) {
				errors = true;
			}
			uid++;
		}
		} else {
			/* Only change records for existing users.
			 */
			struct passwd *pwent;

			setpwent ();
			while ( (pwent = getpwent ()) != NULL ) {
				if (   uflg
				    && (   (has_umin && (pwent->pw_uid < (uid_t)umin))
				        || (has_umax && (pwent->pw_uid > (uid_t)umax)))) {
					continue;
				}
				if (setmax_one (pwent->pw_uid, max)) {
					errors = true;
				}
			}
			endpwent ();
		}
	}
}

/*
 * set_locktime_one - Set the locktime for one user
 *
 * This returns a boolean indicating if an error occurred.
 */
static bool set_locktime_one (uid_t uid, long locktime)
{
	off_t offset;
	struct faillog fl;

	offset = (off_t) uid * sizeof (fl);
	if (offset <= (statbuf.st_size - sizeof (fl))) {
		/* fseeko errors are not really relevant for us. */
		int err = fseeko (fail, offset, SEEK_SET);
		assert (0 == err);
		/* faillog is a sparse file. Even if no entries were
		 * entered for this user, which should be able to get the
		 * empty entry in this case.
		 */
		if (fread ((char *) &fl, sizeof (fl), 1, fail) != 1) {
			fprintf (stderr,
			         _("faillog: Failed to get the entry for UID %lu\n"),
			         (unsigned long int)uid);
			return true;
		}
	} else {
		/* Outsize of the faillog file.
		 * Behave as if there were a missing entry (same behavior
		 * as if we were reading an non existing entry in the
		 * sparse faillog file).
		 */
		memzero (&fl, sizeof (fl));
	}

	if (locktime == fl.fail_locktime) {
		/* If the max is already set to the right value, do not
		 * write in the file.
		 * This avoids writing 0 when no entries were present for
		 * the user and the max argument is 0.
		 */
		return false;
	}

	fl.fail_locktime = locktime;

	if (   (fseeko (fail, offset, SEEK_SET) == 0)
	    && (fwrite ((char *) &fl, sizeof (fl), 1, fail) == 1)) {
		(void) fflush (fail);
		return false;
	}

	fprintf (stderr,
	         _("faillog: Failed to set locktime for UID %lu\n"),
	         (unsigned long int)uid);
	return true;
}

static void set_locktime (long locktime)
{
	if (uflg && has_umin && has_umax && (umin==umax)) {
		if (set_locktime_one ((uid_t)umin, locktime)) {
			errors = true;
		}
	} else {
		/* Set locktime for entries in the specified range.
		 * If locktime is unchanged for an entry, the entry is not touched.
		 * If locktime is null, and no entries exist for this user, no
		 * entries will be created.
		 */
		if (aflg) {
		/* Entries for non existing user are also taken into
		 * account (in order to define policy for future users).
		 */
		uid_t uid = 0;
		/* The default umax value is based on the size of the
		 * faillog database.
		 */
		uid_t uidmax = statbuf.st_size / sizeof (struct faillog);
		if (uidmax > 1) {
			uidmax--;
		}

		/* Make sure we stay in the umin-umax range if specified */
		if (has_umin) {
			uid = (uid_t)umin;
		}
		if (has_umax) {
			uidmax = (uid_t)umax;
		}

		while (uid <= uidmax) {
			if (set_locktime_one (uid, locktime)) {
				errors = true;
			}
			uid++;
		}
		} else {
			/* Only change records for existing users.
			 */
			struct passwd *pwent;

			setpwent ();
			while ( (pwent = getpwent ()) != NULL ) {
				if (   uflg
				    && (   (has_umin && (pwent->pw_uid < (uid_t)umin))
				        || (has_umax && (pwent->pw_uid > (uid_t)umax)))) {
					continue;
				}
				if (set_locktime_one (pwent->pw_uid, locktime)) {
					errors = true;
				}
			}
			endpwent ();
		}
	}
}

int main (int argc, char **argv)
{
	long fail_locktime;
	long fail_max;
	long days;

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

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
		while ((c = getopt_long (argc, argv, "ahl:m:rt:u:",
		                         long_options, &option_index)) != -1) {
			switch (c) {
			case 'a':
				aflg = true;
				break;
			case 'h':
				usage (E_SUCCESS);
				break;
			case 'l':
				if (getlong (optarg, &fail_locktime) == 0) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         "faillog", optarg);
					exit (E_BAD_ARG);
				}
				lflg = true;
				break;
			case 'm':
				if (getlong (optarg, &fail_max) == 0) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         "faillog", optarg);
					exit (E_BAD_ARG);
				}
				mflg = true;
				break;
			case 'r':
				rflg = true;
				break;
			case 't':
				if (getlong (optarg, &days) == 0) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         "faillog", optarg);
					exit (E_BAD_ARG);
				}
				seconds = (time_t) days * DAY;
				tflg = true;
				break;
			case 'u':
			{
				/*
				 * The user can be:
				 *  - a login name
				 *  - numerical
				 *  - a numerical login ID
				 *  - a range (-x, x-, x-y)
				 */
				struct passwd *pwent;

				uflg = true;
				/* local, no need for xgetpwnam */
				pwent = getpwnam (optarg);
				if (NULL != pwent) {
					umin = (unsigned long) pwent->pw_uid;
					has_umin = true;
					umax = umin;
					has_umax = true;
				} else {
					if (getrange (optarg,
					              &umin, &has_umin,
					              &umax, &has_umax) == 0) {
						fprintf (stderr,
						         _("lastlog: Unknown user or range: %s\n"),
						         optarg);
						exit (E_BAD_ARG);
					}
				}

				break;
			}
			default:
				usage (E_USAGE);
			}
		}
	}

	if (tflg && (lflg || mflg || rflg)) {
		usage (E_USAGE);
	}

	/* Open the faillog database */
	if (lflg || mflg || rflg) {
		fail = fopen (FAILLOG_FILE, "r+");
	} else {
		fail = fopen (FAILLOG_FILE, "r");
	}
	if (NULL == fail) {
		fprintf (stderr,
		         _("faillog: Cannot open %s: %s\n"),
		         FAILLOG_FILE, strerror (errno));
		exit (E_NOPERM);
	}

	/* Get the size of the faillog */
	if (fstat (fileno (fail), &statbuf) != 0) {
		fprintf (stderr,
		         _("faillog: Cannot get the size of %s: %s\n"),
		         FAILLOG_FILE, strerror (errno));
		exit (E_NOPERM);
	}

	if (lflg) {
		set_locktime (fail_locktime);
	}

	if (mflg) {
		setmax (fail_max);
	}

	if (rflg) {
		reset ();
	}

	if (!(lflg || mflg || rflg)) {
		print ();
	}

	fclose (fail);

	exit (errors ? E_NOPERM : E_SUCCESS);
}

