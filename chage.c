/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#ifndef	lint
static	char	sccsid[] = "@(#)chage.c	3.12	08:07:05	19 Jul 1993";
#endif

/*
 * Set up some BSD defines so that all the BSD ifdef's are
 * kept right here 
 */

#ifndef	BSD
#include <string.h>
#include <memory.h>
#define	bzero(a,n)	memset(a, 0, n)
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

#include "config.h"
#include "pwd.h"

/*
 * chage depends on some form of aging being present.  It makes no sense
 * to have a program that has no input.
 */

#ifdef	SHADOWPWD
#include "shadow.h"
#ifndef	AGING
#define	AGING
#endif	/* AGING */
#else	/* !SHADOWPWD */
#if !defined(ATT_AGE) && defined(AGING)
#undef	AGING
#endif	/* !ATT_AGE && AGING */
#endif	/* SHADOWPWD */

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif	/* !LOG_WARN */
#endif	/* USE_SYSLOG */

#ifdef	AGING

/*
 * Global variables
 */

char	*Prog;
long	mindays;
long	maxdays;
long	lastday;
#ifdef	SHADOWPWD
long	warndays;
long	inactdays;
long	expdays;
#endif
void	cleanup();

/*
 * External identifiers
 */

extern	long	a64l();
extern	char	*l64a();
extern	int	pw_lock(), pw_open(),
		pw_unlock(), pw_close(),
		pw_update();
extern	struct	passwd	*pw_locate();
#ifdef	SHADOWPWD
extern	int	spw_lock(), spw_open(),
		spw_unlock(), spw_close(),
		spw_update();
extern	struct	spwd	*spw_locate();
#endif
extern	int	optind;
extern	char	*optarg;
extern	char	*getlogin ();
#ifdef	NDBM
extern	int	pw_dbm_mode;
#ifdef	SHADOWPWD
extern	int	sp_dbm_mode;
#endif
#endif

/*
 * Password aging constants
 *
 *	DAY - seconds in a day
 *	WEEK - seconds in a week
 *	SCALE - convert from clock to aging units
 */

#define	DAY	(24L*3600L)
#define	WEEK	(7*DAY)

#ifdef	ITI_AGING
#define	SCALE	(1)
#else
#define	SCALE	(DAY)
#endif

#if !defined(MDY_DATE) && !defined(DMY_DATE) && !defined(YMD_DATE)
#define	MDY_DATE	1
#endif
#if (defined (MDY_DATE) && (defined (DMY_DATE) || defined (YMD_DATE))) || \
    (defined (DMY_DATE) && (defined (MDY_DATE) || defined (YMD_DATE)))
Error: You must only define one of MDY_DATE, DMY_DATE, or YMD_DATE
#endif

/*
 * days and juldays are used to compute the number of days in the
 * current month, and the cummulative number of days in the preceding
 * months.  they are declared so that january is 1, not 0.
 */

static	short	days[13] = { 0,
	31,	28,	31,	30,	31,	30,	/* JAN - JUN */
	31,	31,	30,	31,	30,	31 };	/* JUL - DEC */

static	short	juldays[13] = { 0,
	0,	31,	59,	90,	120,	151,	/* JAN - JUN */
	181,	212,	243,	273,	304,	334 };	/* JUL - DEC */

/*
 * #defines for messages.  This facilitates foreign language conversion
 * since all messages are defined right here.
 */

#ifdef	SHADOWPWD
#define	USAGE \
"Usage: %s [ -l ] [ -m min_days ] [ -M max_days ] [ -W warn ]\n\
       [ -I inactive ] [ -E expire ] [ -d last_day ] user\n"
#else
#define	USAGE \
"Usage: %s [ -l ] [ -m min_days ] [ -M max_days ] [ -d last_day ] user\n"
#endif
#define	DBMERROR	"Error updating the DBM password entry.\n"
#define	UNK_USER	"%s: unknown user: %s\n"
#define	NO_LFLAG	"%s: do no include \"l\" with other flags\n"
#define	NO_PERM		"%s: permission denied\n"
#define	NO_PWLOCK	"%s: can't lock password file\n"
#define	NO_PWOPEN	"%s: can't open password file\n"
#define	CHANGE_INFO	"Changing the aging information for %s\n"
#define	AGE_CHANGED	"changed password expiry for %s\n"
#define	FIELD_ERR	"%s: error changing fields\n"
#define	NO_PWUPDATE	"%s: can't update password file\n"
#define	NO_PWCLOSE	"%s: can't rewrite password file\n"
#define	LOCK_FAIL	"failed locking %s\n"
#define	OPEN_FAIL	"failed opening %s\n"
#define	WRITE_FAIL	"failed updating %s\n"
#define	CLOSE_FAIL	"failed rewriting %s\n"
#ifdef	MDY_DATE
#define	LAST_CHG	"Last Password Change (MM/DD/YY)"
#ifdef	SHADOWPWD
#define	ACCT_EXP	"Account Expiration Date (MM/DD/YY)"
#endif
#define	EPOCH		"12/31/69"
#endif
#ifdef	DMY_DATE
#define	LAST_CHG	"Last Password Change (DD/MM/YY)"
#ifdef	SHADOWPWD
#define	ACCT_EXP	"Account Expiration Date (DD/MM/YY)"
#endif
#define	EPOCH		"31/12/69"
#endif
#ifdef	YMD_DATE
#define	LAST_CHG	"Last Password Change (YY/MM/DD)"
#ifdef	SHADOWPWD
#define	ACCT_EXP	"Account Expiration Date (YY/MM/DD)"
#endif
#define	EPOCH		"69/12/31"
#endif
#ifdef	SHADOWPWD
#define	DBMERROR2	"error updating DBM shadow entry.\n"
#define	NO_SPLOCK	"%s: can't lock shadow password file\n"
#define	NO_SPOPEN	"%s: can't open shadow password file\n"
#define	NO_SPUPDATE	"%s: can't update shadow password file\n"
#define	NO_SPCLOSE	"%s: can't rewrite shadow password file\n"
#else
#define	DBMERROR2	"error updating DBM passwd entry.\n"
#endif

/*
 * usage - print command line syntax and exit
 */

void
usage ()
{
	fprintf (stderr, USAGE, Prog);
	exit (1);
}

/*
 * strtoday - compute the number of days since 1970.
 *
 * the total number of days prior to the current date is
 * computed.  january 1, 1970 is used as the origin with
 * it having a day number of 0.  the gmtime() routine is
 * used to prevent confusion regarding time zones.
 */

long
strtoday (str)
char	*str;
{
	char	slop[2];
	int	month;
	int	day;
	int	year;
	long	total;

	/*
	 * start by separating the month, day and year.  the order
	 * is compiled in ...
	 */

#ifdef	MDY_DATE
	if (sscanf (str, "%d/%d/%d%c", &month, &day, &year, slop) != 3)
		return -1;
#endif
#ifdef	DMY_DATE
	if (sscanf (str, "%d/%d/%d%c", &day, &month, &year, slop) != 3)
		return -1;
#endif
#ifdef	YMD_DATE
	if (sscanf (str, "%d/%d/%d%c", &year, &month, &day, slop) != 3)
		return -1;
#endif

	/*
	 * the month, day of the month, and year are checked for
	 * correctness and the year adjusted so it falls between
	 * 1970 and 2069.
	 */

	if (month < 1 || month > 12)
		return -1;

	if (day < 1)
		return -1;

	if ((month != 2 || (year % 4) != 0) && day > days[month])
		return -1;
	else if ((month == 2 && (year % 4) == 0) && day > 29)
		return -1;

	if (year < 0)
		return -1;
	else if (year < 69)
		year += 2000;
	else if (year < 99)
		year += 1900;

	if (year < 1970 || year > 2069)
		return -1;

	/*
	 * the total number of days is the total number of days in all
	 * the whole years, plus the number of leap days, plus the
	 * number of days in the whole months preceding, plus the number
	 * of days so far in the month.
	 */

	total = ((year - 1970) * 365) + (((year + 1) - 1970) / 4);
	total += juldays[month] + (month > 2 && (year % 4) == 0 ? 1:0);
	total += day - 1;

	return total;
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

int
new_fields ()
{
	char	buf[BUFSIZ];
	char	*cp;
	long	value;
	struct	tm	*tp;

	printf ("Enter the new value, or press return for the default\n\n");

	sprintf (buf, "%ld", mindays);
	change_field (buf, "Minimum Password Age");
	if (((mindays = strtol (buf, &cp, 10)) == 0 && *cp) || mindays < -1)
		return 0;

	sprintf (buf, "%ld", maxdays);
	change_field (buf, "Maximum Password Age");
	if (((maxdays = strtol (buf, &cp, 10)) == 0 && *cp) || maxdays < -1)
		return 0;

	value = lastday * SCALE;
	tp = gmtime (&value);
	sprintf (buf, "%02d/%02d/%02d",
#ifdef	MDY_DATE
		tp->tm_mon + 1, tp->tm_mday, tp->tm_year
#endif
#ifdef	DMY_DATE
		tp->tm_mday, tp->tm_mon + 1, tp->tm_year
#endif
#ifdef	YMD_DATE
		tp->tm_year, tp->tm_mon + 1, tp->tm_mday
#endif
		);

	change_field (buf, LAST_CHG);
	if (strcmp (buf, EPOCH) == 0)
		lastday = -1;
	else if ((lastday = strtoday (buf)) == -1)
		return 0;

#ifdef	SHADOWPWD
	sprintf (buf, "%ld", warndays);
	change_field (buf, "Password Expiration Warning");
	if (((warndays = strtol (buf, &cp, 10)) == 0 && *cp) || warndays < -1)
		return 0;

	sprintf (buf, "%ld", inactdays);
	change_field (buf, "Password Inactive");
	if (((inactdays = strtol (buf, &cp, 10)) == 0 && *cp) || inactdays < -1)
		return 0;

	value = expdays * SCALE;
	tp = gmtime (&value);
	sprintf (buf, "%02d/%02d/%02d",
#ifdef	MDY_DATE
		tp->tm_mon + 1, tp->tm_mday, tp->tm_year
#endif
#ifdef	DMY_DATE
		tp->tm_mday, tp->tm_mon + 1, tp->tm_year
#endif
#ifdef	YMD_DATE
		tp->tm_year, tp->tm_mon + 1, tp->tm_mday
#endif
		);

	change_field (buf, ACCT_EXP);
	if (strcmp (buf, EPOCH) == 0)
		expdays = -1;
	else if ((expdays = strtoday (buf)) == -1)
		return 0;
#endif	/* SHADOWPWD */

	return 1;
}

/*
 * list_fields - display the current values of the expiration fields
 *
 * display the password age information from the password fields.  date
 * values will be displayed as a calendar date, or the word "Never" if
 * the date is 1/1/70, which is day number 0.
 */

void
list_fields ()
{
	struct	tm	*tp;
	char	*cp;
	long	changed;
	long	expires;

	/*
	 * Start with the easy numbers - the number of days before the
	 * password can be changed, the number of days after which the
	 * password must be chaged, the number of days before the
	 * password expires that the user is told, and the number of
	 * days after the password expires that the account becomes
	 * unusable.
	 */

	printf ("Minimum:\t%d\n", mindays);
	printf ("Maximum:\t%d\n", maxdays);
#ifdef	SHADOWPWD
	printf ("Warning:\t%d\n", warndays);
	printf ("Inactive:\t%d\n", inactdays);
#endif

	/*
	 * The "last change" date is either "Never" or the date the
	 * password was last modified.  The date is the number of
	 * days since 1/1/1970.
	 */

	printf ("Last Change:\t\t");
	if (lastday <= 0) {
		printf ("Never\n");
	} else {
		changed = lastday * SCALE;
		tp = gmtime (&changed);
		cp = asctime (tp);
		printf ("%6.6s, %4.4s\n", cp + 4, cp + 20);
	}

	/*
	 * The password expiration date is determined from the last
	 * change date plus the number of days the password is valid
	 * for.
	 */

	printf ("Password Expires:\t");
	if (lastday <= 0 || maxdays >= 10000*(DAY/SCALE) || maxdays <= 0) {
		printf ("Never\n");
	} else {
		expires = changed + maxdays * SCALE;
		tp = gmtime (&expires);
		cp = asctime (tp);
		printf ("%6.6s, %4.4s\n", cp + 4, cp + 20);
	}

#ifdef	SHADOWPWD
	/*
	 * The account becomes inactive if the password is expired
	 * for more than "inactdays".  The expiration date is calculated
	 * and the number of inactive days is added.  The resulting date
	 * is when the active will be disabled.
	 */

	printf ("Password Inactive:\t");
	if (lastday <= 0 || inactdays <= 0 ||
			maxdays >= 10000*(DAY/SCALE) || maxdays <= 0) {
		printf ("Never\n");
	} else {
		expires = changed + (maxdays + inactdays) * SCALE;
		tp = gmtime (&expires);
		cp = asctime (tp);
		printf ("%6.6s, %4.4s\n", cp + 4, cp + 20);
	}

	/*
	 * The account will expire on the given date regardless of the
	 * password expiring or not.
	 */

	printf ("Account Expires:\t");
	if (expdays <= 0) {
		printf ("Never\n");
	} else {
		expires = expdays * SCALE;
		tp = gmtime (&expires);
		cp = asctime (tp);
		printf ("%6.6s, %4.4s\n", cp + 4, cp + 20);
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
main (argc, argv)
int	argc;
char	**argv;
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
	struct	spwd	*sp;
	struct	spwd	spwd;
#else
	char	new_age[5];
#endif
	int	ruid = getuid ();
	struct	passwd	*pw;
	struct	passwd	pwent;
	char	name[BUFSIZ];

	/*
	 * Get the program name so that error messages can use it.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif
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

#ifdef	SHADOWPWD
	while ((flag = getopt (argc, argv, "lm:M:W:I:E:d:")) != EOF)
#else
	while ((flag = getopt (argc, argv, "lm:M:d:")) != EOF)
#endif
	{
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
				if (strchr (optarg, '/'))
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
				if (strchr (optarg, '/'))
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
		fprintf (stderr, NO_LFLAG, Prog);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		usage ();
	}

	/*
	 * An unprivileged user can ask for their own aging information,
	 * but only root can change it, or list another user's aging
	 * information.
	 */

	if (ruid != 0 && ! lflg) {
		fprintf (stderr, NO_PERM, Prog);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Lock and open the password file.  This loads all of the
	 * password file entries into memory.  Then we get a pointer
	 * to the password file entry for the requested user.
	 */

	if (! pw_lock ()) {
		fprintf (stderr, NO_PWLOCK, Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, LOCK_FAIL, "/etc/passwd");
		closelog ();
#endif
		exit (1);
	}
	if (! pw_open (ruid != 0 || lflg ? O_RDONLY:O_RDWR)) {
		fprintf (stderr, NO_PWOPEN, Prog);
		cleanup (1);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, OPEN_FAIL, "/etc/passwd");
		closelog ();
#endif
		exit (1);
	}
	if (! (pw = pw_locate (argv[optind]))) {
		fprintf (stderr, UNK_USER, Prog, argv[optind]);
		cleanup (1);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

#ifdef	SHADOWPWD
	/*
	 * For shadow password files we have to lock the file and
	 * read in the entries as was done for the password file.
	 * The user entries does not have to exist in this case;
	 * a new entry will be created for this user if one does
	 * not exist already.
	 */

	if (! spw_lock ()) {
		fprintf (stderr, NO_SPLOCK, Prog);
		cleanup (1);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, LOCK_FAIL, "/etc/shadow");
		closelog ();
#endif
		exit (1);
	}
	if (! spw_open ((ruid != 0 || lflg) ? O_RDONLY:O_RDWR)) {
		fprintf (stderr, NO_SPOPEN, Prog);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, OPEN_FAIL, "/etc/shadow");
		closelog ();
#endif
		exit (1);
	}
	if (sp = spw_locate (argv[optind]))
		spwd = *sp;
#endif	/* SHADOWPWD */

	strcpy (name, pw->pw_name);
	pwent = *pw;

	/*
	 * Set the fields that aren't being set from the command line
	 * from the password file.
	 */

#ifdef	SHADOWPWD
	if (sp) {
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
		if (ruid != 0 && ruid != pw->pw_uid) {
			fprintf (stderr, NO_PERM, Prog);
#ifdef	USE_SYSLOG
			closelog ();
#endif
			exit (1);
		}
		list_fields ();
		cleanup (2);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (0);
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
		printf (CHANGE_INFO, name);
		if (! new_fields ()) {
			fprintf (stderr, FIELD_ERR, Prog);
			cleanup (2);
#ifdef	USE_SYSLOG
			closelog ();
#endif
			exit (1);
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
		bzero (&spwd, sizeof spwd);

		sp->sp_namp = strdup (pw->pw_name);
		sp->sp_pwdp = strdup (pw->pw_passwd);
		sp->sp_flag = -1;

		pwent.pw_passwd = "!";
#ifdef	ATT_AGE
		pwent.pw_age = "";
#endif
		if (! pw_update (&pwent)) {
			fprintf (stderr, NO_PWUPDATE, Prog);
			cleanup (2);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, WRITE_FAIL, "/etc/passwd");
			closelog ();
#endif
			exit (1);
		}
#if defined(DBM) || defined(NDBM)
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

	sp->sp_max = maxdays;
	sp->sp_min = mindays;
	sp->sp_lstchg = lastday;
	sp->sp_warn = warndays;
	sp->sp_inact = inactdays;
	sp->sp_expire = expdays;

	if (! spw_update (sp)) {
		fprintf (stderr, NO_SPUPDATE, Prog);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, WRITE_FAIL, "/etc/shadow");
		closelog ();
#endif
		exit (1);
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
	pw->pw_age = new_age;

	if (! pw_update (pw)) {
		fprintf (stderr, NO_PWUPDATE, Prog);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, WRITE_FAIL, "/etc/passwd");
		closelog ();
#endif
		exit (1);
	}
#endif	/* SHADOWPWD */

#ifdef	NDBM
#ifdef	SHADOWPWD

	/*
	 * See if the shadow DBM file exists and try to update it.
	 */

	if (access ("/etc/shadow.pag", 0) == 0 && ! sp_dbm_update (sp)) {
		fprintf (stderr, DBMERROR);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, DBMERROR2);
		closelog ();
#endif
		exit (1);
	}
	endspent ();

#else	/* !SHADOWPWD */

	/*
	 * See if the password DBM file exists and try to update it.
	 */

	if (access ("/etc/passwd.pag", 0) == 0 && ! pw_dbm_update (pw)) {
		fprintf (stderr, DBMERROR);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, DBMERROR2);
		closelog ();
#endif
		exit (1);
	}
	endpwent ();
#endif	/* SHADOWPWD */
#endif	/* NDBM */

#ifdef	SHADOWPWD
	/*
	 * Now close the shadow password file, which will cause all
	 * of the entries to be re-written.
	 */

	if (! spw_close ()) {
		fprintf (stderr, NO_SPCLOSE, Prog);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, CLOSE_FAIL, "/etc/shadow");
		closelog ();
#endif
		exit (1);
	}
#endif	/* SHADOWPWD */

	/*
	 * Close the password file.  If any entries were modified, the
	 * file will be re-written.
	 */

	if (! pw_close ()) {
		fprintf (stderr, NO_PWCLOSE, Prog);
		cleanup (2);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, CLOSE_FAIL, "/etc/passwd");
		closelog ();
#endif
		exit (1);
	}
	cleanup (2);
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, AGE_CHANGED, name);
	closelog ();
#endif
	exit (0);
	/*NOTREACHED*/
}

/*
 * cleanup - unlock any locked password files
 */

void
cleanup (state)
int	state;
{
	switch (state) {
		case 2:
#ifdef	SHADOWPWD
			spw_unlock ();
#endif
		case 1:
			pw_unlock ();
		case 0:
			break;
	}
}

#else	/* !AGING */

/*
 * chage - but there is no age info!
 */

main (argc, argv)
int	argc;
char	**argv;
{
	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

	fprintf (stderr, "%s: no aging information present\n", Prog);
	exit (1);
}

#endif	/* AGING */
