/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2000 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2011, Nicolas François
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
#include <lastlog.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include "defines.h"
#include "prototypes.h"
#include "getdef.h"
/*@-exitarg@*/
#include "exitcodes.h"

/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif

/*
 * Global variables
 */
const char *Prog;		/* Program name */
static FILE *lastlogfile;	/* lastlog file stream */
static unsigned long umin;	/* if uflg and has_umin, only display users with uid >= umin */
static bool has_umin = false;
static unsigned long umax;	/* if uflg and has_umax, only display users with uid <= umax */
static bool has_umax = false;
static time_t seconds;		/* that number of days in seconds */
static time_t inverse_seconds;	/* that number of days in seconds */
static struct stat statbuf;	/* fstat buffer for file size */


static bool uflg = false;	/* print only an user of range of users */
static bool tflg = false;	/* print is restricted to most recent days */
static bool bflg = false;	/* print excludes most recent days */
static bool Cflg = false;	/* clear record for user */
static bool Sflg = false;	/* set record for user */

#define	NOW	(time ((time_t *) 0))

static /*@noreturn@*/void usage (int status)
{
	FILE *usageout = (E_SUCCESS != status) ? stderr : stdout;
	(void) fprintf (usageout,
	                _("Usage: %s [options]\n"
	                  "\n"
	                  "Options:\n"),
	                Prog);
	(void) fputs (_("  -b, --before DAYS             print only lastlog records older than DAYS\n"), usageout);
	(void) fputs (_("  -C, --clear                   clear lastlog record of an user (usable only with -u)\n"), usageout);
	(void) fputs (_("  -h, --help                    display this help message and exit\n"), usageout);
	(void) fputs (_("  -R, --root CHROOT_DIR         directory to chroot into\n"), usageout);
	(void) fputs (_("  -S, --set                     set lastlog record to current time (usable only with -u)\n"), usageout);
	(void) fputs (_("  -t, --time DAYS               print only lastlog records more recent than DAYS\n"), usageout);
	(void) fputs (_("  -u, --user LOGIN              print lastlog record of the specified LOGIN\n"), usageout);
	(void) fputs ("\n", usageout);
	exit (status);
}

static void print_one (/*@null@*/const struct passwd *pw)
{
	static bool once = false;
	char *cp;
	struct tm *tm;
	time_t ll_time;
	off_t offset;
	struct lastlog ll;

#ifdef HAVE_STRFTIME
	char ptime[80];
#endif

	if (NULL == pw) {
		return;
	}


	offset = (off_t) pw->pw_uid * sizeof (ll);
	if (offset + sizeof (ll) <= statbuf.st_size) {
		/* fseeko errors are not really relevant for us. */
		int err = fseeko (lastlogfile, offset, SEEK_SET);
		assert (0 == err);
		/* lastlog is a sparse file. Even if no entries were
		 * entered for this user, which should be able to get the
		 * empty entry in this case.
		 */
		if (fread ((char *) &ll, sizeof (ll), 1, lastlogfile) != 1) {
			fprintf (stderr,
			         _("%s: Failed to get the entry for UID %lu\n"),
			         Prog, (unsigned long int)pw->pw_uid);
			exit (EXIT_FAILURE);
		}
	} else {
		/* Outsize of the lastlog file.
		 * Behave as if there were a missing entry (same behavior
		 * as if we were reading an non existing entry in the
		 * sparse lastlog file).
		 */
		memzero (&ll, sizeof (ll));
	}

	/* Filter out entries that do not match with the -t or -b options */
	if (tflg && ((NOW - ll.ll_time) > seconds)) {
		return;
	}

	if (bflg && ((NOW - ll.ll_time) < inverse_seconds)) {
		return;
	}

	/* Print the header only once */
	if (!once) {
#ifdef HAVE_LL_HOST
		puts (_("Username         Port     From             Latest"));
#else
		puts (_("Username                Port     Latest"));
#endif
		once = true;
	}

	ll_time = ll.ll_time;
	tm = localtime (&ll_time);
#ifdef HAVE_STRFTIME
	strftime (ptime, sizeof (ptime), "%a %b %e %H:%M:%S %z %Y", tm);
	cp = ptime;
#else
	cp = asctime (tm);
	cp[24] = '\0';
#endif

	if (ll.ll_time == (time_t) 0) {
		cp = _("**Never logged in**\0");
	}

#ifdef HAVE_LL_HOST
	printf ("%-16s %-8.8s %-16.16s %s\n",
	        pw->pw_name, ll.ll_line, ll.ll_host, cp);
#else
	printf ("%-16s\t%-8.8s %s\n",
	        pw->pw_name, ll.ll_line, cp);
#endif
}

static void print (void)
{
	const struct passwd *pwent;
	unsigned long lastlog_uid_max;

	lastlog_uid_max = getdef_ulong ("LASTLOG_UID_MAX", 0xFFFFFFFFUL);
	if (   (has_umin && umin > lastlog_uid_max)
	    || (has_umax && umax > lastlog_uid_max)) {
		fprintf (stderr, _("%s: Selected uid(s) are higher than LASTLOG_UID_MAX (%lu),\n"
				   "\tthe output might be incorrect.\n"), Prog, lastlog_uid_max);
	}

	if (uflg && has_umin && has_umax && (umin == umax)) {
		print_one (getpwuid ((uid_t)umin));
	} else {
		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			if (   uflg
			    && (   (has_umin && (pwent->pw_uid < (uid_t)umin))
			        || (has_umax && (pwent->pw_uid > (uid_t)umax)))) {
				continue;
			} else if ( !uflg && pwent->pw_uid > (uid_t) lastlog_uid_max) {
				continue;
			}
			print_one (pwent);
		}
		endpwent ();
	}
}

static void update_one (/*@null@*/const struct passwd *pw)
{
	off_t offset;
	struct lastlog ll;
	int err;

	if (NULL == pw) {
		return;
	}

	offset = (off_t) pw->pw_uid * sizeof (ll);
	/* fseeko errors are not really relevant for us. */
	err = fseeko (lastlogfile, offset, SEEK_SET);
	assert (0 == err);

	memzero (&ll, sizeof (ll));

	if (Sflg) {
		ll.ll_time = NOW;
#ifdef HAVE_LL_HOST
		strcpy (ll.ll_host, "localhost");
#endif
		strcpy (ll.ll_line, "lastlog");
#ifdef WITH_AUDIT
		audit_logger (AUDIT_ACCT_UNLOCK, Prog,
			"clearing-lastlog",
			pw->pw_name, (unsigned int) pw->pw_uid, SHADOW_AUDIT_SUCCESS);
#endif
	}
#ifdef WITH_AUDIT
	else {
		audit_logger (AUDIT_ACCT_UNLOCK, Prog,
			"refreshing-lastlog",
			pw->pw_name, (unsigned int) pw->pw_uid, SHADOW_AUDIT_SUCCESS);
	}
#endif

	if (fwrite (&ll, sizeof(ll), 1, lastlogfile) != 1) {
			fprintf (stderr,
			         _("%s: Failed to update the entry for UID %lu\n"),
			         Prog, (unsigned long int)pw->pw_uid);
			exit (EXIT_FAILURE);
	}
}

static void update (void)
{
	const struct passwd *pwent;
	unsigned long lastlog_uid_max;

	if (!uflg) /* safety measure */
		return;

	lastlog_uid_max = getdef_ulong ("LASTLOG_UID_MAX", 0xFFFFFFFFUL);
	if (   (has_umin && umin > lastlog_uid_max)
	    || (has_umax && umax > lastlog_uid_max)) {
		fprintf (stderr, _("%s: Selected uid(s) are higher than LASTLOG_UID_MAX (%lu),\n"
				   "\tthey will not be updated.\n"), Prog, lastlog_uid_max);
		return;
	}

	if (has_umin && has_umax && (umin == umax)) {
		update_one (getpwuid ((uid_t)umin));
	} else {
		setpwent ();
		while ( (pwent = getpwent ()) != NULL ) {
			if ((has_umin && (pwent->pw_uid < (uid_t)umin))
				|| (has_umax && (pwent->pw_uid > (uid_t)umax))) {
				continue;
			}
			update_one (pwent);
		}
		endpwent ();
	}

	if (fflush (lastlogfile) != 0 || fsync (fileno (lastlogfile)) != 0) {
			fprintf (stderr,
			         _("%s: Failed to update the lastlog file\n"),
			         Prog);
			exit (EXIT_FAILURE);
	}
}

int main (int argc, char **argv)
{
	/*
	 * Get the program name. The program name is used as a prefix to
	 * most error messages.
	 */
	Prog = Basename (argv[0]);

	(void) setlocale (LC_ALL, "");
	(void) bindtextdomain (PACKAGE, LOCALEDIR);
	(void) textdomain (PACKAGE);

	process_root_flag ("-R", argc, argv);

#ifdef WITH_AUDIT
	audit_help_open ();
#endif

	{
		int c;
		static struct option const longopts[] = {
			{"before", required_argument, NULL, 'b'},
			{"clear",  no_argument,       NULL, 'C'},
			{"help",   no_argument,       NULL, 'h'},
			{"root",   required_argument, NULL, 'R'},
			{"set",    no_argument,       NULL, 'S'},
			{"time",   required_argument, NULL, 't'},
			{"user",   required_argument, NULL, 'u'},
			{NULL, 0, NULL, '\0'}
		};

		while ((c = getopt_long (argc, argv, "b:ChR:St:u:", longopts,
		                         NULL)) != -1) {
			switch (c) {
			case 'b':
			{
				unsigned long inverse_days;
				if (getulong (optarg, &inverse_days) == 0) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					exit (EXIT_FAILURE);
				}
				inverse_seconds = (time_t) inverse_days * DAY;
				bflg = true;
				break;
			}
			case 'C':
			{
				Cflg = true;
				break;
			}
			case 'h':
				usage (EXIT_SUCCESS);
				/*@notreached@*/break;
			case 'R': /* no-op, handled in process_root_flag () */
				break;
			case 'S':
			{
				Sflg = true;
				break;
			}
			case 't':
			{
				unsigned long days;
				if (getulong (optarg, &days) == 0) {
					fprintf (stderr,
					         _("%s: invalid numeric argument '%s'\n"),
					         Prog, optarg);
					exit (EXIT_FAILURE);
				}
				seconds = (time_t) days * DAY;
				tflg = true;
				break;
			}
			case 'u':
			{
				const struct passwd *pwent;
				/*
				 * The user can be:
				 *  - a login name
				 *  - numerical
				 *  - a numerical login ID
				 *  - a range (-x, x-, x-y)
				 */
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
						         _("%s: Unknown user or range: %s\n"),
						         Prog, optarg);
						exit (EXIT_FAILURE);
					}
				}
				break;
			}
			default:
				usage (EXIT_FAILURE);
				/*@notreached@*/break;
			}
		}
		if (argc > optind) {
			fprintf (stderr,
			         _("%s: unexpected argument: %s\n"),
			         Prog, argv[optind]);
			usage (EXIT_FAILURE);
		}
		if (Cflg && Sflg) {
			fprintf (stderr,
			         _("%s: Option -C cannot be used together with option -S\n"),
			         Prog);
			usage (EXIT_FAILURE);
		}
		if ((Cflg || Sflg) && !uflg) {
			fprintf (stderr,
			         _("%s: Options -C and -S require option -u to specify the user\n"),
			         Prog);
			usage (EXIT_FAILURE);
		}
	}

	lastlogfile = fopen (LASTLOG_FILE, (Cflg || Sflg)?"r+":"r");
	if (NULL == lastlogfile) {
		perror (LASTLOG_FILE);
		exit (EXIT_FAILURE);
	}

	/* Get the lastlog size */
	if (fstat (fileno (lastlogfile), &statbuf) != 0) {
		fprintf (stderr,
		         _("%s: Cannot get the size of %s: %s\n"),
		         Prog, LASTLOG_FILE, strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (Cflg || Sflg)
		update ();
	else
		print ();

	(void) fclose (lastlogfile);

	return EXIT_SUCCESS;
}

