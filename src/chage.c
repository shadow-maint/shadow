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
RCSID(PKG_VER "$Id: chage.c,v 1.18 2000/09/02 18:40:43 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include "prototypes.h"
#include "defines.h"

#include <pwd.h>

/*
 * chage depends on some form of aging being present.  It makes no sense
 * to have a program that has no input.
 */

#ifdef	SHADOWPWD
#ifndef	AGING
#define	AGING
#endif	/* AGING */
#else	/* !SHADOWPWD */
#if !defined(ATT_AGE) && defined(AGING)
#undef	AGING
#endif	/* !ATT_AGE && AGING */
#endif	/* SHADOWPWD */

static char	*Prog;
static int amroot;

#ifdef	AGING	/*{*/

/*
 * Global variables
 */

static long	mindays;
static long	maxdays;
static long	lastday;
#ifdef	SHADOWPWD
static long	warndays;
static long	inactdays;
static long	expdays;
#endif

/*
 * External identifiers
 */

extern	long	a64l();
extern	char	*l64a();

#include "pwio.h"

#ifdef	SHADOWPWD
#include "shadowio.h"
#endif

extern	int	optind;
extern	char	*optarg;
#ifdef	NDBM
extern	int	pw_dbm_mode;
#ifdef	SHADOWPWD
extern	int	sp_dbm_mode;
#endif
#endif

/*
 * #defines for messages.  This facilitates foreign language conversion
 * since all messages are defined right here.
 */

/*
 * xgettext doesn't like #defines, so now we only leave untranslated
 * messages here.  -MM
 */

#define	AGE_CHANGED	"changed password expiry for %s\n"
#define	LOCK_FAIL	"failed locking %s\n"
#define	OPEN_FAIL	"failed opening %s\n"
#define	WRITE_FAIL	"failed updating %s\n"
#define	CLOSE_FAIL	"failed rewriting %s\n"

#define	EPOCH		"1969-12-31"

#ifdef	SHADOWPWD
#define	DBMERROR2	"error updating DBM shadow entry.\n"
#else
#define	DBMERROR2	"error updating DBM passwd entry.\n"
#endif

/* local function prototypes */
static void usage(void);
static void date_to_str(char *, size_t, time_t);
static int new_fields(void);
static void print_date(time_t);
static void list_fields(void);
static void cleanup(int);


/*
 * isnum - determine whether or not a string is a number
 */
int
isnum(const char *s)
{
	while (*s) {
		if (!isdigit(*s))
			return 0;
		s++;
	}
	return 1;
}

/*
 * usage - print command line syntax and exit
 */

static void
usage(void)
{
#ifdef SHADOWPWD
   fprintf(stderr, _("Usage: %s [ -l ] [ -m min_days ] [ -M max_days ] [ -W warn ]\n  [ -I inactive ] [ -E expire ] [ -d last_day ] user\n"), Prog);
#else
   fprintf(stderr, _("Usage: %s [ -l ] [ -m min_days ] [ -M max_days ] [ -d last_day ] user\n"), Prog);
#endif
	exit(1);
}

static void
date_to_str(char *buf, size_t maxsize, time_t date)
{
	struct tm *tp;

	tp = gmtime(&date);
#ifdef HAVE_STRFTIME
	strftime(buf, maxsize, "%Y-%m-%d", tp);
#else
	snprintf(buf, maxsize, "%04d-%02d-%02d",
		tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday);
#endif /* HAVE_STRFTIME */
}

/*
 * new_fields - change the user's password aging information interactively.
 *
 * prompt the user for all of the password age values.  set the fields
 * from the user's response, or leave alone if nothing was entered.  the
 * value (-1) is used to indicate the field should be removed if possible.
 * any other negative value is an error.  very large positive values will
 * be handled elsewhere.
 */

static int
new_fields(void)
{
	char	buf[200];
	char	*cp;

	printf(_("Enter the new value, or press return for the default\n\n"));

	snprintf(buf, sizeof buf, "%ld", mindays);
	change_field(buf, sizeof buf, _("Minimum Password Age"));
	if (((mindays = strtol (buf, &cp, 10)) == 0 && *cp) || mindays < -1)
		return 0;

	snprintf(buf, sizeof buf, "%ld", maxdays);
	change_field(buf, sizeof buf, _("Maximum Password Age"));
	if (((maxdays = strtol (buf, &cp, 10)) == 0 && *cp) || maxdays < -1)
		return 0;

	date_to_str(buf, sizeof buf, lastday * SCALE);

	change_field(buf, sizeof buf, _("Last Password Change (YYYY-MM-DD)"));

	if (strcmp (buf, EPOCH) == 0)
		lastday = -1;
	else if ((lastday = strtoday (buf)) == -1)
		return 0;

#ifdef	SHADOWPWD
	snprintf(buf, sizeof buf, "%ld", warndays);
	change_field (buf, sizeof buf, _("Password Expiration Warning"));
	if (((warndays = strtol (buf, &cp, 10)) == 0 && *cp) || warndays < -1)
		return 0;

	snprintf(buf, sizeof buf, "%ld", inactdays);
	change_field(buf, sizeof buf, _("Password Inactive"));
	if (((inactdays = strtol (buf, &cp, 10)) == 0 && *cp) || inactdays < -1)
		return 0;

	date_to_str(buf, sizeof buf, expdays * SCALE);

	change_field(buf, sizeof buf, _("Account Expiration Date (YYYY-MM-DD)"));

	if (strcmp (buf, EPOCH) == 0)
		expdays = -1;
	else if ((expdays = strtoday (buf)) == -1)
		return 0;
#endif	/* SHADOWPWD */

	return 1;
}

static void
print_date(time_t date)
{
#ifdef HAVE_STRFTIME
	struct tm *tp;
	char buf[80];

	tp = gmtime(&date);
	strftime(buf, sizeof buf, "%b %d, %Y", tp);
	puts(buf);
#else
	struct tm *tp;
	char *cp;

	tp = gmtime(&date);
	cp = asctime(tp);
	printf("%6.6s, %4.4s\n", cp + 4, cp + 20);
#endif
}

/*
 * list_fields - display the current values of the expiration fields
 *
 * display the password age information from the password fields.  date
 * values will be displayed as a calendar date, or the word "Never" if
 * the date is 1/1/70, which is day number 0.
 */

static void
list_fields(void)
{
	long changed = 0;
	long expires;

	/*
	 * Start with the easy numbers - the number of days before the
	 * password can be changed, the number of days after which the
	 * password must be chaged, the number of days before the
	 * password expires that the user is told, and the number of
	 * days after the password expires that the account becomes
	 * unusable.
	 */

	printf(_("Minimum:\t%ld\n"), mindays);
	printf(_("Maximum:\t%ld\n"), maxdays);
#ifdef SHADOWPWD
	printf(_("Warning:\t%ld\n"), warndays);
	printf(_("Inactive:\t%ld\n"), inactdays);
#endif

	/*
	 * The "last change" date is either "Never" or the date the
	 * password was last modified.  The date is the number of
	 * days since 1/1/1970.
	 */

	printf(_("Last Change:\t\t"));
	if (lastday <= 0) {
		printf(_("Never\n"));
	} else {
		changed = lastday * SCALE;
		print_date(changed);
	}

	/*
	 * The password expiration date is determined from the last
	 * change date plus the number of days the password is valid
	 * for.
	 */

	printf(_("Password Expires:\t"));
	if (lastday <= 0 || maxdays >= 10000*(DAY/SCALE) || maxdays <= 0) {
		printf (_("Never\n"));
	} else {
		expires = changed + maxdays * SCALE;
		print_date(expires);
	}

#ifdef	SHADOWPWD
	/*
	 * The account becomes inactive if the password is expired
	 * for more than "inactdays".  The expiration date is calculated
	 * and the number of inactive days is added.  The resulting date
	 * is when the active will be disabled.
	 */

	printf(_("Password Inactive:\t"));
	if (lastday <= 0 || inactdays <= 0 ||
			maxdays >= 10000*(DAY/SCALE) || maxdays <= 0) {
		printf (_("Never\n"));
	} else {
		expires = changed + (maxdays + inactdays) * SCALE;
		print_date(expires);
	}

	/*
	 * The account will expire on the given date regardless of the
	 * password expiring or not.
	 */

	printf(_("Account Expires:\t"));
	if (expdays <= 0) {
		printf (_("Never\n"));
	} else {
		expires = expdays * SCALE;
		print_date(expires);
	}
#endif
}

/*
 * chage - change a user's password aging information
 *
 *	This command controls the password aging information.
 *
 *	The valid options are
 *
 *	-m	minimum number of days before password change (*)
 *	-M	maximim number of days before password change (*)
 *	-d	last password change date (*)
 *	-l	password aging information
 *	-W	expiration warning days (*)
 *	-I	password inactive after expiration (*)
 *	-E	account expiration date (*)
 *
 *	(*) requires root permission to execute.
 *
 *	All of the time fields are entered in the internal format
 *	which is either seconds or days.
 *
 *	The options -W, -I and -E all depend on the SHADOWPWD
 *	macro being defined.
 */

int
main(int argc, char **argv)
{
	int	flag;
	int	lflg = 0;
	int	mflg = 0;
	int	Mflg = 0;
	int	dflg = 0;
#ifdef	SHADOWPWD
	int	Wflg = 0;
	int	Iflg = 0;
	int	Eflg = 0;
	const struct spwd *sp;
	struct spwd spwd;
#else
	char	new_age[5];
#endif
	uid_t ruid;
	const struct passwd *pw;
	struct passwd pwent;
	char	name[BUFSIZ];

	sanitize_env();
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	ruid = getuid();
	amroot = (ruid == 0);

	/*
	 * Get the program name so that error messages can use it.
	 */

	Prog = Basename(argv[0]);

	OPENLOG("chage");
#ifdef	NDBM
#ifdef	SHADOWPWD
	sp_dbm_mode = O_RDWR;
#endif
	pw_dbm_mode = O_RDWR;
#endif

	/*
	 * Parse the flags.  The difference between password file
	 * formats includes the number of fields, and whether the
	 * dates are entered as days or weeks.  Shadow password
	 * file info =must= be entered in days, while regular
	 * password file info =must= be entered in weeks.
	 */

#ifdef SHADOWPWD
#define FLAGS "lm:M:W:I:E:d:"
#else
#define FLAGS "lm:M:d:"
#endif
	while ((flag = getopt(argc, argv, FLAGS)) != EOF) {
#undef FLAGS
		switch (flag) {
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
			case 'd':
				dflg++;
				if (!isnum(optarg))
					lastday = strtoday (optarg);
				else
					lastday = strtol (optarg, 0, 10);
				break;
#ifdef	SHADOWPWD
			case 'W':
				Wflg++;
				warndays = strtol (optarg, 0, 10);
				break;
			case 'I':
				Iflg++;
				inactdays = strtol (optarg, 0, 10);
				break;
			case 'E':
				Eflg++;
				if (!isnum(optarg))
					expdays = strtoday (optarg);
				else
					expdays = strtol (optarg, 0, 10);
				break;
#endif
			default:
				usage ();
		}
	}

	/*
	 * Make certain the flags do not conflict and that there is
	 * a user name on the command line.
	 */

	if (argc != optind + 1)
		usage ();

#ifdef	SHADOWPWD
	if (lflg && (mflg || Mflg || dflg || Wflg || Iflg || Eflg))
#else
	if (lflg && (mflg || Mflg || dflg))
#endif
	{
		fprintf (stderr, _("%s: do not include \"l\" with other flags\n"), Prog);
		closelog();
		usage ();
	}

	/*
	 * An unprivileged user can ask for their own aging information,
	 * but only root can change it, or list another user's aging
	 * information.
	 */

	if (!amroot && !lflg) {
		fprintf (stderr, _("%s: permission denied\n"), Prog);
		closelog();
		exit (1);
	}

	/*
	 * Lock and open the password file.  This loads all of the
	 * password file entries into memory.  Then we get a pointer
	 * to the password file entry for the requested user.
	 */
	/* We don't lock the password file if we are not root */
	if (amroot && !pw_lock()) {
		fprintf(stderr, _("%s: can't lock password file\n"), Prog);
		SYSLOG((LOG_ERR, LOCK_FAIL, PASSWD_FILE));
		closelog();
		exit(1);
	}
	if (!pw_open((!amroot || lflg) ? O_RDONLY:O_RDWR)) {
		fprintf(stderr, _("%s: can't open password file\n"), Prog);
		cleanup(1);
		SYSLOG((LOG_ERR, OPEN_FAIL, PASSWD_FILE));
		closelog();
		exit(1);
	}
	if (!(pw = pw_locate(argv[optind]))) {
		fprintf(stderr, _("%s: unknown user: %s\n"), Prog, argv[optind]);
		cleanup(1);
		closelog();
		exit(1);
	}

	pwent = *pw;
	STRFCPY(name, pwent.pw_name);

#ifdef	SHADOWPWD
	/*
	 * For shadow password files we have to lock the file and
	 * read in the entries as was done for the password file.
	 * The user entries does not have to exist in this case;
	 * a new entry will be created for this user if one does
	 * not exist already.
	 */
	/* We don't lock the shadow file if we are not root */
	if (amroot && !spw_lock()) {
		fprintf(stderr, _("%s: can't lock shadow password file\n"), Prog);
		cleanup(1);
		SYSLOG((LOG_ERR, LOCK_FAIL, SHADOW_FILE));
		closelog();
		exit(1);
	}
	if (!spw_open((!amroot || lflg) ? O_RDONLY : O_RDWR)) {
		fprintf(stderr, _("%s: can't open shadow password file\n"), Prog);
		cleanup(2);
		SYSLOG((LOG_ERR, OPEN_FAIL, SHADOW_FILE));
		closelog();
		exit(1);
	}

	sp = spw_locate(argv[optind]);

	/*
	 * Set the fields that aren't being set from the command line
	 * from the password file.
	 */

	if (sp) {
		spwd = *sp;

		if (! Mflg)
			maxdays = spwd.sp_max;
		if (! mflg)
			mindays = spwd.sp_min;
		if (! dflg)
			lastday = spwd.sp_lstchg;
		if (! Wflg)
			warndays = spwd.sp_warn;
		if (! Iflg)
			inactdays = spwd.sp_inact;
		if (! Eflg)
			expdays = spwd.sp_expire;
	}
#ifdef	ATT_AGE
	else
#endif	/* ATT_AGE */
#endif	/* SHADOWPWD */
#ifdef	ATT_AGE
	{
		if (pwent.pw_age && strlen (pwent.pw_age) >= 2) {
			if (! Mflg)
				maxdays = c64i (pwent.pw_age[0]) * (WEEK/SCALE);
			if (! mflg)
				mindays = c64i (pwent.pw_age[1]) * (WEEK/SCALE);
			if (! dflg && strlen (pwent.pw_age) == 4)
				lastday = a64l (pwent.pw_age+2) * (WEEK/SCALE);
		} else {
			mindays = 0;
			maxdays = 10000L * (DAY/SCALE);
			lastday = -1;
		}
#ifdef	SHADOWPWD
		warndays = inactdays = expdays = -1;
#endif	/* SHADOWPWD */
	}
#endif	/* ATT_AGE */

	/*
	 * Print out the expiration fields if the user has
	 * requested the list option.
	 */

	if (lflg) {
		if (!amroot && (ruid != pwent.pw_uid)) {
			fprintf(stderr, _("%s: permission denied\n"), Prog);
			closelog();
			exit(1);
		}
		list_fields();
		cleanup(2);
		closelog();
		exit(0);
	}

	/*
	 * If none of the fields were changed from the command line,
	 * let the user interactively change them.
	 */

#ifdef	SHADOWPWD
	if (! mflg && ! Mflg && ! dflg && ! Wflg && ! Iflg && ! Eflg)
#else
	if (! mflg && ! Mflg && ! dflg)
#endif
	{
		printf(_("Changing the aging information for %s\n"), name);
		if (!new_fields()) {
			fprintf(stderr, _("%s: error changing fields\n"), Prog);
			cleanup(2);
			closelog();
			exit(1);
		}
	}

#ifdef	SHADOWPWD
	/*
	 * There was no shadow entry.  The new entry will have the
	 * encrypted password transferred from the normal password
	 * file along with the aging information.
	 */

	if (sp == 0) {
		sp = &spwd;
		memzero(&spwd, sizeof spwd);

		spwd.sp_namp = xstrdup (pwent.pw_name);
		spwd.sp_pwdp = xstrdup (pwent.pw_passwd);
		spwd.sp_flag = -1;

		pwent.pw_passwd = SHADOW_PASSWD_STRING;  /* XXX warning: const */
#ifdef	ATT_AGE
		pwent.pw_age = "";
#endif
		if (!pw_update(&pwent)) {
			fprintf(stderr, _("%s: can't update password file\n"), Prog);
			cleanup(2);
			SYSLOG((LOG_ERR, WRITE_FAIL, PASSWD_FILE));
			closelog();
			exit(1);
		}
#ifdef NDBM
		(void) pw_dbm_update (&pwent);
		endpwent ();
#endif
	}
#endif	/* SHADOWPWD */

#ifdef	SHADOWPWD

	/*
	 * Copy the fields back to the shadow file entry and
	 * write the modified entry back to the shadow file.
	 * Closing the shadow and password files will commit
	 * any changes that have been made.
	 */

	spwd.sp_max = maxdays;
	spwd.sp_min = mindays;
	spwd.sp_lstchg = lastday;
	spwd.sp_warn = warndays;
	spwd.sp_inact = inactdays;
	spwd.sp_expire = expdays;

	if (!spw_update(&spwd)) {
		fprintf(stderr, _("%s: can't update shadow password file\n"), Prog);
		cleanup(2);
		SYSLOG((LOG_ERR, WRITE_FAIL, SHADOW_FILE));
		closelog();
		exit(1);
	}
#else	/* !SHADOWPWD */

	/*
	 * fill in the new_age string with the new values
	 */

	if (maxdays > (63 * 7) && mindays == 0) {
		new_age[0] = '\0';
	} else {
		if (maxdays > (63 * 7))
			maxdays = 63 * 7;

		if (mindays > (63 * 7))
			mindays = 63 * 7;

		new_age[0] = i64c (maxdays / 7);
		new_age[1] = i64c ((mindays + 6) / 7);

		if (lastday == 0)
			new_age[2] = '\0';
		else
			strcpy (new_age + 2, l64a (lastday / 7));

	}
	pwent.pw_age = new_age;

	if (!pw_update(&pwent)) {
		fprintf(stderr, _("%s: can't update password file\n"), Prog);
		cleanup(2);
		SYSLOG((LOG_ERR, WRITE_FAIL, PASSWD_FILE));
		closelog();
		exit(1);
	}
#endif	/* SHADOWPWD */

#ifdef	NDBM
#ifdef	SHADOWPWD

	/*
	 * See if the shadow DBM file exists and try to update it.
	 */

	if (sp_dbm_present() && !sp_dbm_update(&spwd)) {
		fprintf(stderr, _("Error updating the DBM password entry.\n"));
		cleanup(2);
		SYSLOG((LOG_ERR, DBMERROR2));
		closelog();
		exit(1);
	}
	endspent();

#else	/* !SHADOWPWD */

	/*
	 * See if the password DBM file exists and try to update it.
	 */

	if (pw_dbm_present() && !pw_dbm_update(&pwent)) {
		fprintf(stderr, _("Error updating the DBM password entry.\n"));
		cleanup(2);
		SYSLOG((LOG_ERR, DBMERROR2));
		closelog();
		exit(1);
	}
	endpwent ();
#endif	/* SHADOWPWD */
#endif	/* NDBM */

#ifdef	SHADOWPWD
	/*
	 * Now close the shadow password file, which will cause all
	 * of the entries to be re-written.
	 */

	if (!spw_close()) {
		fprintf(stderr, _("%s: can't rewrite shadow password file\n"), Prog);
		cleanup(2);
		SYSLOG((LOG_ERR, CLOSE_FAIL, SHADOW_FILE));
		closelog();
		exit(1);
	}
#endif	/* SHADOWPWD */

	/*
	 * Close the password file.  If any entries were modified, the
	 * file will be re-written.
	 */

	if (!pw_close()) {
		fprintf(stderr, _("%s: can't rewrite password file\n"), Prog);
		cleanup(2);
		SYSLOG((LOG_ERR, CLOSE_FAIL, PASSWD_FILE));
		closelog();
		exit(1);
	}
	cleanup(2);
	SYSLOG((LOG_INFO, AGE_CHANGED, name));
	closelog();
	exit(0);
	/*NOTREACHED*/
}

/*
 * cleanup - unlock any locked password files
 */

static void
cleanup(int state)
{
	switch (state) {
	case 2:
#ifdef	SHADOWPWD
		if (amroot)
			spw_unlock();
#endif
	case 1:
		if (amroot)
			pw_unlock();
	case 0:
		break;
	}
}

#else	/*} !AGING {*/

/*
 * chage - but there is no age info!
 */

int
main(int argc, char **argv)
{
	char	*Prog;

	Prog = Basename(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fprintf (stderr, _("%s: no aging information present\n"), Prog);
	exit(1);
}

#endif	/*} AGING */

