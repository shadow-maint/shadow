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

#ident "$Id$"

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#ifdef USE_PAM
#include "pam_defs.h"
#endif				/* USE_PAM */
#include <pwd.h>
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#include <selinux/av_permissions.h>
#endif
#include "exitcodes.h"
#include "prototypes.h"
#include "defines.h"
#include "pwio.h"
#include "shadowio.h"
/*
 * Global variables
 */
static char *Prog;

static int
 dflg = 0,			/* set last password change date */
    Eflg = 0,			/* set account expiration date */
    Iflg = 0,			/* set password inactive after expiration */
    lflg = 0,			/* show account aging information */
    mflg = 0,			/* set minimum number of days before password change */
    Mflg = 0,			/* set maximim number of days before password change */
    Wflg = 0;			/* set expiration warning days */
static int amroot = 0;

static long mindays;
static long maxdays;
static long lastday;
static long warndays;
static long inactdays;
static long expdays;

#define	EPOCH		"1969-12-31"

/* local function prototypes */
static void usage (void);
static void date_to_str (char *, size_t, time_t);
static int new_fields (void);
static void print_date (time_t);
static void list_fields (void);
static void process_flags (int argc, char **argv);
static void check_flags (int argc, int opt_index);
static void check_perms (void);
static void open_files (int readonly);
static void close_files (void);

/*
 * isnum - determine whether or not a string is a number
 */
int isnum (const char *s)
{
	while ('\0' != *s) {
		if (!isdigit (*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

/*
 * usage - print command line syntax and exit
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: chage [options] [LOGIN]\n"
	                   "\n"
	                   "Options:\n"
	                   "  -d, --lastday LAST_DAY        set last password change to LAST_DAY\n"
	                   "  -E, --expiredate EXPIRE_DATE  set account expiration date to EXPIRE_DATE\n"
	                   "  -h, --help                    display this help message and exit\n"
	                   "  -I, --inactive INACTIVE       set password inactive after expiration\n"
	                   "                                to INACTIVE\n"
	                   "  -l, --list                    show account aging information\n"
	                   "  -m, --mindays MIN_DAYS        set minimum number of days before password\n"
	                   "                                change to MIN_DAYS\n"
	                   "  -M, --maxdays MAX_DAYS        set maximim number of days before password\n"
	                   "                                change to MAX_DAYS\n"
	                   "  -W, --warndays WARN_DAYS      set expiration warning days to WARN_DAYS\n"
	                   "\n"));
	exit (E_USAGE);
}

static void date_to_str (char *buf, size_t maxsize, time_t date)
{
	struct tm *tp;

	tp = gmtime (&date);
#ifdef HAVE_STRFTIME
	strftime (buf, maxsize, "%Y-%m-%d", tp);
#else
	snprintf (buf, maxsize, "%04d-%02d-%02d",
	          tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
#endif				/* HAVE_STRFTIME */
}

/*
 * new_fields - change the user's password aging information interactively.
 *
 * prompt the user for all of the password age values. set the fields
 * from the user's response, or leave alone if nothing was entered. The
 * value (-1) is used to indicate the field should be removed if possible.
 * any other negative value is an error. very large positive values will
 * be handled elsewhere.
 */
static int new_fields (void)
{
	char buf[200];
	char *cp;

	printf (_("Enter the new value, or press ENTER for the default\n"));
	printf ("\n");

	snprintf (buf, sizeof buf, "%ld", mindays);
	change_field (buf, sizeof buf, _("Minimum Password Age"));
	if (((mindays = strtol (buf, &cp, 10)) == 0 && ('\0' != *cp))
	    || (mindays < -1)) {
		return 0;
	}

	snprintf (buf, sizeof buf, "%ld", maxdays);
	change_field (buf, sizeof buf, _("Maximum Password Age"));
	if (((maxdays = strtol (buf, &cp, 10)) == 0 && ('\0' != *cp))
	    || (maxdays < -1)) {
		return 0;
	}

	date_to_str (buf, sizeof buf, lastday * SCALE);

	change_field (buf, sizeof buf, _("Last Password Change (YYYY-MM-DD)"));

	if (strcmp (buf, EPOCH) == 0) {
		lastday = -1;
	} else if ((lastday = strtoday (buf)) == -1) {
		return 0;
	}

	snprintf (buf, sizeof buf, "%ld", warndays);
	change_field (buf, sizeof buf, _("Password Expiration Warning"));
	if (((warndays = strtol (buf, &cp, 10)) == 0 && ('\0' != *cp))
	    || (warndays < -1)) {
		return 0;
	}

	snprintf (buf, sizeof buf, "%ld", inactdays);
	change_field (buf, sizeof buf, _("Password Inactive"));
	if (((inactdays = strtol (buf, &cp, 10)) == 0 && ('\0' != *cp))
	    || (inactdays < -1)) {
		return 0;
	}

	date_to_str (buf, sizeof buf, expdays * SCALE);

	change_field (buf, sizeof buf,
	              _("Account Expiration Date (YYYY-MM-DD)"));

	if (strcmp (buf, EPOCH) == 0) {
		expdays = -1;
	} else if ((expdays = strtoday (buf)) == -1) {
		return 0;
	}

	return 1;
}

static void print_date (time_t date)
{
#ifdef HAVE_STRFTIME
	struct tm *tp;
	char buf[80];

	tp = gmtime (&date);
	strftime (buf, sizeof buf, "%b %d, %Y", tp);
	puts (buf);
#else
	struct tm *tp;
	char *cp;

	tp = gmtime (&date);
	cp = asctime (tp);
	printf ("%6.6s, %4.4s\n", cp + 4, cp + 20);
#endif
}

/*
 * list_fields - display the current values of the expiration fields
 *
 * display the password age information from the password fields. Date
 * values will be displayed as a calendar date, or the word "never" if
 * the date is 1/1/70, which is day number 0.
 */
static void list_fields (void)
{
	long changed = 0;
	long expires;

	/*
	 * The "last change" date is either "never" or the date the password
	 * was last modified. The date is the number of days since 1/1/1970.
	 */
	printf (_("Last password change\t\t\t\t\t: "));
	if (lastday < 0) {
		printf (_("never\n"));
	} else if (lastday == 0) {
		printf (_("password must be changed\n"));
	} else {
		changed = lastday * SCALE;
		print_date (changed);
	}

	/*
	 * The password expiration date is determined from the last change
	 * date plus the number of days the password is valid for.
	 */
	printf (_("Password expires\t\t\t\t\t: "));
	if ((lastday <= 0) || (maxdays >= (10000 * (DAY / SCALE)))
	    || (maxdays < 0)) {
		printf (_("never\n"));
	} else {
		expires = changed + maxdays * SCALE;
		print_date (expires);
	}

	/*
	 * The account becomes inactive if the password is expired for more
	 * than "inactdays". The expiration date is calculated and the
	 * number of inactive days is added. The resulting date is when the
	 * active will be disabled.
	 */
	printf (_("Password inactive\t\t\t\t\t: "));
	if ((lastday <= 0) || (inactdays < 0) ||
	    (maxdays >= (10000 * (DAY / SCALE))) || (maxdays < 0)) {
		printf (_("never\n"));
	} else {
		expires = changed + (maxdays + inactdays) * SCALE;
		print_date (expires);
	}

	/*
	 * The account will expire on the given date regardless of the
	 * password expiring or not.
	 */
	printf (_("Account expires\t\t\t\t\t\t: "));
	if (expdays < 0) {
		printf (_("never\n"));
	} else {
		expires = expdays * SCALE;
		print_date (expires);
	}

	/*
	 * Start with the easy numbers - the number of days before the
	 * password can be changed, the number of days after which the
	 * password must be chaged, the number of days before the password
	 * expires that the user is told, and the number of days after the
	 * password expires that the account becomes unusable.
	 */
	printf (_("Minimum number of days between password change\t\t: %ld\n"),
	        mindays);
	printf (_("Maximum number of days between password change\t\t: %ld\n"),
	        maxdays);
	printf (_("Number of days of warning before password expires\t: %ld\n"),
	        warndays);
}

static void process_flags (int argc, char **argv)
{
	/*
	 * Parse the command line options.
	 */
	int option_index = 0;
	int c;
	static struct option long_options[] = {
		{"lastday", required_argument, NULL, 'd'},
		{"expiredate", required_argument, NULL, 'E'},
		{"help", no_argument, NULL, 'h'},
		{"inactive", required_argument, NULL, 'I'},
		{"list", no_argument, NULL, 'l'},
		{"mindays", required_argument, NULL, 'm'},
		{"maxdays", required_argument, NULL, 'M'},
		{"warndays", required_argument, NULL, 'W'},
		{NULL, 0, NULL, '\0'}
	};

	while ((c =
		getopt_long (argc, argv, "d:E:hI:lm:M:W:", long_options,
			     &option_index)) != -1) {
		switch (c) {
		case 'd':
			dflg++;
			if (!isnum (optarg)) {
				lastday = strtoday (optarg);
			} else {
				lastday = strtol (optarg, 0, 10);
			}
			break;
		case 'E':
			Eflg++;
			if (!isnum (optarg)) {
				expdays = strtoday (optarg);
			} else {
				expdays = strtol (optarg, 0, 10);
			}
			break;
		case 'h':
			usage ();
			break;
		case 'I':
			Iflg++;
			inactdays = strtol (optarg, 0, 10);
			break;
		case 'l':
			lflg++;
			break;
		case 'm':
			mflg++;
			mindays = strtol (optarg, 0, 10);
			break;
		case 'M':
			Mflg++;
			maxdays = strtol (optarg, 0, 10);
			break;
		case 'W':
			Wflg++;
			warndays = strtol (optarg, 0, 10);
			break;
		default:
			usage ();
		}
	}

	check_flags (argc, optind);
}


static void check_flags (int argc, int opt_index)
{

	/*
	 * Make certain the flags do not conflict and that there is a user
	 * name on the command line.
	 */

	if (argc != opt_index + 1) {
		usage ();
	}

	if (lflg && (mflg || Mflg || dflg || Wflg || Iflg || Eflg)) {
		fprintf (stderr,
		         _("%s: do not include \"l\" with other flags\n"),
		         Prog);
		usage ();
	}
}

/* Additional check done later */
static void check_perms (void)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	struct passwd *pampw;
	int retval;
#endif

	/*
	 * An unprivileged user can ask for their own aging information, but
	 * only root can change it, or list another user's aging
	 * information.
	 */

	if (!amroot && !lflg) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age", NULL,
		              getuid (), 0);
#endif
		exit (E_NOPERM);
	}

#ifdef USE_PAM
	retval = PAM_SUCCESS;

	pampw = getpwuid (getuid ()); /* local, no need for xgetpwuid */
	if (pampw == NULL) {
		retval = PAM_USER_UNKNOWN;
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_start ("chage", pampw->pw_name, &conv, &pamh);
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval == PAM_SUCCESS) {
		retval = pam_acct_mgmt (pamh, 0);
		if (retval != PAM_SUCCESS) {
			pam_end (pamh, retval);
		}
	}

	if (retval != PAM_SUCCESS) {
		fprintf (stderr, _("%s: PAM authentication failed\n"), Prog);
		exit (E_NOPERM);
	}
#endif				/* USE_PAM */
}

static void open_files (int readonly)
{
	/*
	 * open the password file. This loads all of the password
	 * file entries into memory. Then we get a pointer to the password
	 * file entry for the requested user.
	 */
	if (pw_open (O_RDONLY) == 0) {
		fprintf (stderr, _("%s: can't open password file\n"), Prog);
		SYSLOG ((LOG_ERR, "failed opening %s", PASSWD_FILE));
		closelog ();
		exit (E_NOPERM);
	}

	/*
	 * For shadow password files we have to lock the file and read in
	 * the entries as was done for the password file. The user entries
	 * does not have to exist in this case; a new entry will be created
	 * for this user if one does not exist already.
	 */
	if (!readonly && (spw_lock () == 0)) {
		fprintf (stderr,
		         _("%s: can't lock shadow password file\n"), Prog);
		SYSLOG ((LOG_ERR, "failed locking %s", SHADOW_FILE));
		closelog ();
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age", name,
		              getuid (), 0);
#endif
		exit (E_NOPERM);
	}
	if (spw_open (readonly ? O_RDONLY: O_RDWR) == 0) {
		fprintf (stderr,
		         _("%s: can't open shadow password file\n"), Prog);
		spw_unlock ();
		SYSLOG ((LOG_ERR, "failed opening %s", SHADOW_FILE));
		closelog ();
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age", name,
		              getuid (), 0);
#endif
		exit (E_NOPERM);
	}
}

static void close_files (void)
{
	/*
	 * Now close the shadow password file, which will cause all of the
	 * entries to be re-written.
	 */
	if (spw_close () == 0) {
		fprintf (stderr,
		         _("%s: can't rewrite shadow password file\n"), Prog);
		spw_unlock ();
		SYSLOG ((LOG_ERR, "failed rewriting %s", SHADOW_FILE));
		closelog ();
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
		              pw->pw_name, getuid (), 0);
#endif
		exit (E_NOPERM);
	}

	/*
	 * Close the password file. If any entries were modified, the file
	 * will be re-written.
	 */
	if (pw_close () == 0) {
		fprintf (stderr, _("%s: can't rewrite password file\n"), Prog);
		spw_unlock ();
		SYSLOG ((LOG_ERR, "failed rewriting %s", PASSWD_FILE));
		closelog ();
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
		              pw->pw_name, getuid (), 0);
#endif
		exit (E_NOPERM);
	}
	spw_unlock ();
}

/*
 * chage - change a user's password aging information
 *
 *	This command controls the password aging information.
 *
 *	The valid options are
 *
 *	-d	set last password change date (*)
 *	-E	set account expiration date (*)
 *	-I	set password inactive after expiration (*)
 *	-l	show account aging information
 *	-M	set maximim number of days before password change (*)
 *	-m	set minimum number of days before password change (*)
 *	-W	set expiration warning days (*)
 *
 *	(*) requires root permission to execute.
 *
 *	All of the time fields are entered in the internal format which is
 *	either seconds or days.
 */

int main (int argc, char **argv)
{
	const struct spwd *sp;
	struct spwd spwent;
	uid_t ruid;
	gid_t rgid;
	const struct passwd *pw;
	struct passwd pwent;
	char name[BUFSIZ];

#ifdef WITH_AUDIT
	audit_help_open ();
#endif
	sanitize_env ();
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	ruid = getuid ();
	rgid = getgid ();
	amroot = (ruid == 0);
#ifdef WITH_SELINUX
	if (amroot && (is_selinux_enabled () > 0)) {
		amroot = (selinux_check_passwd_access (PASSWD__ROOTOK) == 0);
	}
#endif

	/*
	 * Get the program name so that error messages can use it.
	 */
	Prog = Basename (argv[0]);

	process_flags (argc, argv);

	OPENLOG ("chage");

	check_perms ();

	if (!spw_file_present ()) {
		fprintf (stderr,
		         _("%s: the shadow password file is not present\n"),
		         Prog);
		SYSLOG ((LOG_ERR, "can't find the shadow password file"));
		closelog ();
		exit (E_SHADOW_NOTFOUND);
	}

	open_files (lflg);

	if (!(pw = pw_locate (argv[optind]))) {
		fprintf (stderr, _("%s: unknown user %s\n"), Prog,
		         argv[optind]);
		closelog ();
		exit (E_NOPERM);
	}

	pwent = *pw;
	STRFCPY (name, pwent.pw_name);

	/* Drop privileges */
	if (lflg && (setregid (rgid, rgid) || setreuid (ruid, ruid))) {
		fprintf (stderr, _("%s: failed to drop privileges (%s)\n"),
		         Prog, strerror (errno));
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age", name,
		              getuid (), 0);
#endif
		exit (E_NOPERM);
	}

	sp = spw_locate (argv[optind]);

	/*
	 * Set the fields that aren't being set from the command line from
	 * the password file.
	 */
	if (NULL != sp) {
		spwent = *sp;

		if (!Mflg) {
			maxdays = spwent.sp_max;
		}
		if (!mflg) {
			mindays = spwent.sp_min;
		}
		if (!dflg) {
			lastday = spwent.sp_lstchg;
		}
		if (!Wflg) {
			warndays = spwent.sp_warn;
		}
		if (!Iflg) {
			inactdays = spwent.sp_inact;
		}
		if (!Eflg) {
			expdays = spwent.sp_expire;
		}
#ifdef WITH_AUDIT
		if (Mflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change max age", pw->pw_name, pw->pw_uid,
			              1);
		}
		if (mflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change min age", pw->pw_name, pw->pw_uid,
			              1);
		}
		if (dflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change last change date", pw->pw_name,
			              pw->pw_uid, 1);
		}
		if (Wflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change passwd warning", pw->pw_name,
			              pw->pw_uid, 1);
		}
		if (Iflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change inactive days", pw->pw_name,
			              pw->pw_uid, 1);
		}
		if (Eflg) {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change passwd expiration", pw->pw_name,
			              pw->pw_uid, 1);
		}
#endif
	}

	/*
	 * Print out the expiration fields if the user has requested the
	 * list option.
	 */

	if (lflg) {
		if (!amroot && (ruid != pwent.pw_uid)) {
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
			              pw->pw_name, pw->pw_uid, 0);
#endif
			fprintf (stderr, _("%s: Permission denied.\n"), Prog);
			closelog ();
			exit (E_NOPERM);
		}
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "display aging info",
		              pw->pw_name, pw->pw_uid, 1);
#endif
		list_fields ();
		spw_unlock ();
		closelog ();
		exit (E_SUCCESS);
	}

	/*
	 * If none of the fields were changed from the command line, let the
	 * user interactively change them.
	 */
	if (!mflg && !Mflg && !dflg && !Wflg && !Iflg && !Eflg) {
		printf (_("Changing the aging information for %s\n"), name);
		if (new_fields () == 0) {
			fprintf (stderr, _("%s: error changing fields\n"),
			         Prog);
			spw_unlock ();
			closelog ();
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
			              pw->pw_name, getuid (), 0);
#endif
			exit (E_NOPERM);
		}
#ifdef WITH_AUDIT
		else {
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog,
			              "change all aging information",
			              pw->pw_name, getuid (), 1);
		}
#endif
	}
	/*
	 * There was no shadow entry. The new entry will have the encrypted
	 * password transferred from the normal password file along with the
	 * aging information.
	 */
	if (NULL == sp) {
		sp = &spwent;
		memzero (&spwent, sizeof spwent);

		spwent.sp_namp = xstrdup (pwent.pw_name);
		spwent.sp_pwdp = xstrdup (pwent.pw_passwd);
		spwent.sp_flag = -1;

		pwent.pw_passwd = SHADOW_PASSWD_STRING;	/* XXX warning: const */
		if (pw_update (&pwent) == 0) {
			fprintf (stderr,
			         _("%s: can't update password file\n"), Prog);
			spw_unlock ();
			SYSLOG ((LOG_ERR, "failed updating %s", PASSWD_FILE));
			closelog ();
#ifdef WITH_AUDIT
			audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
			              pw->pw_name, getuid (), 0);
#endif
			exit (E_NOPERM);
		}
	}

	/*
	 * Copy the fields back to the shadow file entry and write the
	 * modified entry back to the shadow file. Closing the shadow and
	 * password files will commit any changes that have been made.
	 */
	spwent.sp_max = maxdays;
	spwent.sp_min = mindays;
	spwent.sp_lstchg = lastday;
	spwent.sp_warn = warndays;
	spwent.sp_inact = inactdays;
	spwent.sp_expire = expdays;

	if (spw_update (&spwent) == 0) {
		fprintf (stderr,
		         _("%s: can't update shadow password file\n"), Prog);
		spw_unlock ();
		SYSLOG ((LOG_ERR, "failed updating %s", SHADOW_FILE));
		closelog ();
#ifdef WITH_AUDIT
		audit_logger (AUDIT_USER_CHAUTHTOK, Prog, "change age",
		              pw->pw_name, getuid (), 0);
#endif
		exit (E_NOPERM);
	}

	close_files ();

	SYSLOG ((LOG_INFO, "changed password expiry for %s", name));

#ifdef USE_PAM
	pam_end (pamh, PAM_SUCCESS);
#endif				/* USE_PAM */

	closelog ();
	exit (E_SUCCESS);
}

