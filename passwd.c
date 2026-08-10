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

#include "config.h"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#ifndef	lint
static	char	sccsid[] = "@(#)passwd.c	3.11	08:57:44	10 Jun 1993";
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

#ifdef	STDLIB_H
#include <stdlib.h>
#endif
#ifdef	UNISTD_H
#include <unistd.h>
#endif
#ifdef	ULIMIT_H
#include <ulimit.h>
#endif

#ifndef UL_SFILLIM
#define UL_SFILLIM	2
#endif

#include "pwd.h"
#include "lastlog.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#ifndef	AGING
#define	AGING	0
#endif
#endif
#include "pwauth.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif
#ifdef	HAVE_RLIMIT
#include <sys/resource.h>

struct	rlimit	rlimit_fsize = { RLIM_INFINITY, RLIM_INFINITY };
#endif

#ifdef	AGING

/*
 * Password aging constants
 *
 *	DAY - seconds in a day
 *	WEEK - seconds in a week
 *	SCALE - convert from clock to aging units
 */

#ifndef	DAY
#define	DAY	(24L*3600L)
#endif
#define	WEEK	(7L*DAY)

#ifdef	ITI_AGING
#define	SCALE	(1)
#else
#define	SCALE	DAY
#endif
#endif

/*
 * Global variables
 */

char	name[32];		/* The user's name */
char	crypt_passwd[32];	/* The "old-style" password, if present */
char	*Prog;			/* Program name */
int	amroot;			/* The real UID was 0 */

/*
 * External identifiers
 */

extern	char	*getpass();
extern	char	*pw_encrypt();
extern	int	pw_auth();
extern	char	*getlogin();
extern	char	l64a();
extern	int	optind;		/* Index into argv[] for current option */
extern	char	*optarg;	/* Pointer to current option value */
#ifdef	NDBM
extern	int	sp_dbm_mode;
extern	int	pw_dbm_mode;
#endif

/*
 * #defines for messages.  This facilities foreign language conversion
 * since all messages are defined right here.
 */

#define	USAGE		"usage: %s [ -f | -s ] [ name ]\n"
#define	ADMUSAGE \
	"       %s [ -x max ] [ -n min ] [ -w warn ] [ -i inact ] name\n"
#define	ADMUSAGE2 \
	"       %s { -l | -d | -S } name\n"
#define	OLDPASS		"Old Password:"
#define	NEWPASSMSG \
"Enter the new password (minimum of %d characters)\n\
Please use a combination of upper and lower case letters and numbers.\n"
#define	CHANGING	"Changing password for %s\n"
#define NEWPASS		"New Password:"
#define	NEWPASS2	"Re-enter new password:"
#define	WRONGPWD	"Incorrect password for %s.\n"
#define	WRONGPWD2	"incorrect password for `%s'\n"
#define	NOMATCH		"They don't match; try again.\n"
#define	CANTCHANGE	"The password for %s cannot be changed.\n"
#define	CANTCHANGE2	"password locked for `%s'\n"

#ifdef	AGING
#define	TOOSOON		"Sorry, the password for %s cannot be changed yet.\n"
#define	TOOSOON2	"now < sp_min for `%s'\n"
#endif

#define	EXECFAILED	"%s: Cannot execute %s"
#define	EXECFAILED2	"cannot execute %s\n"
#define	WHOAREYOU	"%s: Cannot determine you user name.\n"
#define	UNKUSER		"%s: Unknown user %s\n"
#define	NOPERM		"You may not change the password for %s.\n"
#define	NOPERM2		"can't change pwd for `%s'\n"
#define	UNCHANGED	"The password for %s is unchanged.\n"

#ifdef	SHADOWPWD
#define	SPWDBUSY	"Cannot lock the password file; try again later.\n"
#define	SPWDBUSY2	"can't lock /etc/shadow\n"
#define	OPNERROR	"Cannot open the password file.\n"
#define	OPNERROR2	"can't open /etc/shadow\n"
#define	UPDERROR	"Error updating the password entry.\n"
#define	UPDERROR2	"error updating shadow entry\n"
#define	DBMERROR	"Error updating the DBM password entry.\n"
#define	DBMERROR2	"error updating DBM shadow entry.\n"
#define	CLSERROR	"Cannot commit shadow file changes.\n"
#define	CLSERROR2	"can't rewrite /etc/shadow.\n"
#define	UNLKERROR	"Cannot unlock the shadow file.\n"
#define	UNLKERROR2	"can't unlock /etc/shadow.\n"
#else
#define	PWDBUSY		"Cannot lock the password file; try again later.\n"
#define	PWDBUSY2	"can't lock /etc/passwd\n"
#define	OPNERROR	"Cannot open the password file.\n"
#define	OPNERROR2	"can't open /etc/passwd\n"
#define	UPDERROR	"Error updating the password entry.\n"
#define	UPDERROR2	"error updating password entry\n"
#define	DBMERROR	"Error updating the DBM password entry.\n"
#define	DBMERROR2	"error updating DBM password entry.\n"
#define	CLSERROR	"Cannot commit password file changes.\n"
#define	CLSERROR2	"can't rewrite /etc/passwd.\n"
#define	UNLKERROR	"Cannot unlock the password file.\n"
#define	UNLKERROR2	"can't unlock /etc/passwd.\n"
#endif

#define	NOTROOT		"Cannot change ID to root.\n"
#define	NOTROOT2	"can't setuid(0).\n"
#define	TRYAGAIN	"Try again.\n"
#define	CHGPASSWD	"changed password for `%s'\n"
#define	NOCHGPASSWD	"did not change password for `%s'\n"

/*
 * usage - print command usage and exit
 */

void
usage ()
{
	fprintf (stderr, USAGE, Prog);
	if (amroot) {
		fprintf (stderr, ADMUSAGE, Prog);
		fprintf (stderr, ADMUSAGE2, Prog);
	}
	exit (1);
}

/*
 * get_password - locate encrypted password in authentication list
 */

char *
get_password (list)
char	*list;
{
	char	*cp, *end;
	static	char	buf[257];

	strcpy (buf, list);
	for (cp = buf;cp;cp = end) {
		if (end = strchr (cp, ';'))
			*end++ = 0;

		if (cp[0] == '@')
			continue;

		return cp;
	}
	return (char *) 0;
}

/*
 * uses_default_method - determine if "old-style" password present
 *
 *	uses_default_method determines if a "old-style" password is present
 *	in the authentication string, and if one is present it extracts it.
 */

int
uses_default_method (methods)
char	*methods;
{
	char	*cp;

	if (cp = get_password (methods)) {
		strcpy (crypt_passwd, cp);
		return 1;
	}
	return 0;
}

/*
 * update_age - update the "last_changed" field
 */

#ifdef	AGING
void
#ifdef	SHADOWPWD
update_age (sp)
struct	spwd	*sp;
{
	sp->sp_lstchg = time ((time_t *) 0) / SCALE;
}
#else
update_age (pw)
struct	passwd	*pw;
{
#ifdef	ATT_AGE
	long	week;		/* at the office ... */
	static	char	age[5];	/* Password age string */

	week = time ((time_t *) 0) / WEEK;
	if (pw->pw_age[0]) {
		cp = l64a (week);
		age[0] = pw->pw_age[0];
		age[1] = pw->pw_age[1];
		age[2] = cp[0];
		age[3] = cp[1];
		age[4] = '\0';
		pw->pw_age = age;
	}
#endif	/* ATT_AGE */
}
#endif	/* SHADOWPWD */
#endif	/* AGING */

/*
 * insert_crypt_passwd - add an "old-style" password to authentication string
 */

insert_crypt_passwd (string, passwd, result)
char	*string;
char	*passwd;
char	*result;
{
	while (*string) {
		if (string[0] == ';') {
			*result++ = *string++;
		} else if (string[0] == '@') {
			while (*string && *string != ';')
				*result++ = *string++;
		} else {
			while (*passwd)
				*result++ = *passwd++;

			while (*string && *string != ';')
				string++;
		}
	}
	*result = '\0';
}

/*
 * new_password - validate old password and replace with new
 */

/*ARGSUSED*/
int
#ifdef	SHADOWPWD
new_password (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
#else
new_password (pw)
struct	passwd	*pw;
#endif
{
	char	*clear;		/* Pointer to clear text */
	char	*cipher;	/* Pointer to cipher text */
	char	*cp;		/* Pointer to getpass() response */
	char	orig[BUFSIZ];	/* Original password */
	char	pass[BUFSIZ];	/* New password */
	char	new_auth[257];
	int	i;		/* Counter for retries */
#ifdef	AGING
	long	week;		/* This week in history ... */
#endif

	/*
	 * Authenticate the user.  The user will be prompted for their
	 * own password.
	 */

	if (! amroot && crypt_passwd[0]) {
		bzero (orig, sizeof orig);

		if (! (clear = getpass (OLDPASS)))
			return -1;

		cipher = pw_encrypt (clear, crypt_passwd);
		if (strcmp (cipher, crypt_passwd) != 0) {
			sleep (1);
			fprintf (stderr, WRONGPWD, pw->pw_name);
#ifdef	USE_SYSLOG
			syslog (LOG_WARN, WRONGPWD2, pw->pw_name);
#endif
			return -1;
		}
		strcpy (orig, clear);
		bzero (cipher, strlen (cipher));
		bzero (clear, strlen (clear));
	}

	/*
	 * Get the new password.  The user is prompted for the new password
	 * and has three tries to get it right.  The password will be tested
	 * for strength, unless it is the root user.  This provides an escape
	 * for initial login passwords.
	 */

	printf (NEWPASSMSG, getdef_num ("PASS_MIN_LEN", 5));
	for (i = 0;i < 3;i++) {
		if (! (cp = getpass (NEWPASS))) {
			bzero (orig, sizeof orig);
			return -1;
		} else
			strcpy (pass, cp);

		if (! amroot && ! obscure (orig, pass)) {
			printf (TRYAGAIN);
			continue;
		}
		if (! (cp = getpass (NEWPASS2))) {
			bzero (orig, sizeof orig);
			return -1;
		}
		if (strcmp (cp, pass))
			fprintf (stderr, NOMATCH);
		else {
			bzero (cp, strlen (cp));
			break;
		}
	}
	bzero (orig, sizeof orig);

	if (i == 3) {
		bzero (pass, sizeof pass);
		return -1;
	}

	/*
	 * Encrypt the password.  The new password is encrypted and
	 * the shadow password structure updated to reflect the change.
	 */

#ifdef	SHADOWPWD
	cp = pw_encrypt (pass, (char *) 0);
	insert_crypt_passwd (sp->sp_pwdp, cp, new_auth);
	cp = sp->sp_pwdp;
	sp->sp_pwdp = strdup (new_auth);
	update_age (sp);
#else
	cp = pw_encrypt (pass, (char *) 0);
	insert_crypt_passwd (pw->pw_passwd, cp, new_auth);
	cp = pw->pw_passwd;
	pw->pw_passwd = strdup (new_auth);
#ifdef	AGING
	update_age (pw);
#endif
#endif	/* SHADOWPWD */
	bzero (pass, sizeof pass);
	bzero (new_auth, sizeof new_auth);
	bzero (cp, strlen (cp));

	return 0;
}

#ifdef	AGING

/*
 * check_password - test a password to see if it can be changed
 *
 *	check_password() sees if the invoker has permission to change the
 *	password for the given user.
 */

/*ARGSUSED*/
void
#ifdef	SHADOWPWD
check_password (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
#else
check_password (pw)
struct	passwd	*pw;
#endif
{
	time_t	now = time ((time_t *) 0) / SCALE;
#ifndef	SHADOWPWD
	time_t	last;
	time_t	ok;
#endif

	/*
	 * Root can change any password any time.
	 */

	if (amroot)
		return;

	/*
	 * Expired accounts cannot be changed ever.  Passwords
	 * which are locked may not be changed.  Passwords where
	 * min > max may not be changed.  Passwords which have
	 * been inactive too long cannot be changed.
	 */

#ifdef	SHADOWPWD
	if ((sp->sp_expire > 0 && now >= sp->sp_expire) ||
	    (sp->sp_inact >= 0 && sp->sp_max >= 0 &&
		now >= (sp->sp_lstchg + sp->sp_inact + sp->sp_max)) ||
			strcmp (sp->sp_pwdp, "!") == 0 ||
			sp->sp_min > sp->sp_max) {
		fprintf (stderr, CANTCHANGE, sp->sp_namp);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, CANTCHANGE2, sp->sp_namp);
		closelog ();
#endif
		exit (1);
	}
#endif	/* SHADOWPWD */

	/*
	 * Passwords may only be changed after sp_min time is up.
	 */

#ifdef	SHADOWPWD
	if (sp->sp_min >= 0 && now < (sp->sp_lstchg + sp->sp_min)) {
		fprintf (stderr, TOOSOON, sp->sp_namp);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, TOOSOON2, sp->sp_namp);
		closelog ();
#endif
		exit (1);
	}
#else	/* !SHADOWPWD */

	/*
	 * Can always be changed if there is no age info
	 */

	if (! pw->pw_age[0])
		return;

	last = a64l (pw->pw_age + 2) * WEEK;
	ok = last + c64i (pw->pw_age[1]) * WEEK;

	if (now < ok) {
		fprintf (stderr, TOOSOON, pw->pw_name);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, TOOSOON2, pw->pw_name);
		closelog ();
#endif
		exit (1);
	}
#endif	/* SHADOWPWD */
}
#endif	/* AGING */

#ifdef	SHADOWPWD

/*
 * pwd_to_spwd - create entries for new spwd structure
 *
 *	pwd_to_spwd() creates a new (struct spwd) containing the
 *	information in the pointed-to (struct passwd).
 */

void
pwd_to_spwd (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
{
	time_t	t;

	/*
	 * Nice, easy parts first.  The name and passwd map directly
	 * from the old password structure to the new one.
	 */

	sp->sp_namp = strdup (pw->pw_name);
	sp->sp_pwdp = strdup (pw->pw_passwd);
#ifdef	ATT_AGE

	/*
	 * AT&T-style password aging maps the sp_min, sp_max, and
	 * sp_lstchg information from the pw_age field, which appears
	 * after the encrypted password.
	 */

	if (pw->pw_age[0]) {
		t = (c64i (pw->pw_age[0]) * WEEK) / SCALE;
		sp->sp_max = t;

		if (pw->pw_age[1]) {
			t = (c64i (pw->pw_age[1]) * WEEK) / SCALE;
			sp->sp_min = t;
		} else
			sp->sp_min = (10000L * DAY) / SCALE;

		if (pw->pw_age[1] && pw->pw_age[2]) {
			t = (a64l (pw->pw_age + 2) * WEEK) / SCALE;
			sp->sp_lstchg = t;
		} else
			sp->sp_lstchg = time ((time_t *) 0) / SCALE;
	} else {
		sp->sp_min = 0;
		sp->sp_max = (10000L * DAY) / SCALE;
		sp->sp_lstchg = time ((time_t *) 0) / SCALE;
	}
#else	/* !ATT_AGE */
	/*
	 * BSD does not use the pw_age field and has no aging information
	 * anywheres.  The default values are used to initialize the
	 * fields which are in the missing pw_age field;
	 */

	sp->sp_min = 0;
	sp->sp_max = (10000L * DAY) / SCALE;
	sp->sp_lstchg = time ((time_t *) 0) / SCALE;
#endif	/* ATT_AGE */

	/*
	 * These fields have no corresponding information in the password
	 * file.  They are set to uninitialized values.
	 */

	sp->sp_warn = -1;
	sp->sp_inact = -1;
	sp->sp_expire = -1;
	sp->sp_flag = -1;
}

#endif	/* SHADOWPWD */

/*
 * print_status - print current password status
 */

/*ARGSUSED*/
void
#ifdef	SHADOWPWD
print_status (pw, sp)
struct	passwd	*pw;
struct	spwd	*sp;
#else
print_status (pw)
struct	passwd	*pw;
#endif
{
#ifdef	AGING
	struct	tm	*tm;
	time_t	last_time;
#endif
#ifdef	SHADOWPWD
	last_time = sp->sp_lstchg * SCALE;
	tm = gmtime (&last_time);

	printf ("%s ", sp->sp_namp);
	printf ("%s ",
		sp->sp_pwdp[0] ? (sp->sp_pwdp[0] == '!' ? "L":"P"):"NP");
	printf ("%02.2d/%02.2d/%02.2d ",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100);
	printf ("%d %d %d %d\n",
		(sp->sp_min * SCALE) / DAY, (sp->sp_max * SCALE) / DAY,
		(sp->sp_warn * SCALE) / DAY, (sp->sp_inact * SCALE) / DAY);
#else
	printf ("%s ", pw->pw_name);
	printf ("%s",
		pw->pw_passwd[0] ? (pw->pw_passwd[0] == '!' ? "L":"P"):"NP");

#ifdef	ATT_AGE
	if (pw->pw_age[0])
		last_time = a64l (pw->pw_age + 2);
	else
		last_time = 0L;

	tm = gmtime (&last_time);
	printf (" %02.2d/%02.2d/%02.2d ",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100);
	printf ("%d %d\n",
		pw->pw_age[0] ? c64i (pw->pw_age[1]) * 7:10000,
		pw->pw_age[0] ? c64i (pw->pw_age[0]) * 7:0);
#else	/* !ATT_AGE */
	putchar ('\n');
#endif	/* ATT_AGE */
#endif	/* SHADOWPWD */
}

/*
 * passwd - change a user's password file information
 *
 *	This command controls the password file and commands which are
 * 	used to modify it.
 *
 *	The valid options are
 *
 *	-l	lock the named account (*)
 *	-d	delete the password for the named account (*)
 *	-x #	set sp_max to # days (*)
 *	-n #	set sp_min to # days (*)
 *	-w #	set sp_warn to # days (*)
 *	-i #	set sp_inact to # days (*)
 *	-S	show password status of named account (*)
 *	-g	execute gpasswd command to interpret flags
 *	-f	execute chfn command to interpret flags
 *	-s	execute chsh command to interpret flags
 *
 *	(*) requires root permission to execute.
 *
 *	All of the time fields are entered in days and converted to the
 * 	appropriate internal format.  For finer resolute the chage
 *	command must be used.
 */

int
main (argc, argv)
int	argc;
char	**argv;
{
	char	buf[BUFSIZ];		/* I/O buffer for messages, etc.      */
	char	new_passwd[BUFSIZ];	/* Buffer for changed passwords       */
	char	*cp;			/* Miscellaneous character pointing   */
	time_t	min;			/* Minimum days before change         */
	time_t	max;			/* Maximum days until change          */
	time_t	warn;			/* Warning days before change         */
	time_t	inact;			/* Days without change before locked  */
	int	i;			/* Loop control variable              */
	int	flag;			/* Current option to process          */
	int	lflg = 0;		/* -l - lock account option           */
	int	uflg = 0;		/* -u - unlock account option         */
	int	dflg = 0;		/* -d - delete password option        */
#ifdef	AGING
	int	xflg = 0;		/* -x - set maximum days              */
	int	nflg = 0;		/* -n - set minimum days              */
	int	wflg = 0;		/* -w - set warning days              */
	int	iflg = 0;		/* -i - set inactive days             */
#endif
	int	Sflg = 0;		/* -S - show password status          */
	struct	passwd	*pw;		/* Password file entry for user       */
#ifdef	SHADOWPWD
	struct	spwd	*sp;		/* Shadow file entry for user         */
	struct	spwd	tspwd;		/* New shadow file entry if none      */
#else
	char	age[5];			/* New password age entry             */
#endif

	/*
	 * The program behaves differently when executed by root
	 * than when executed by a normal user.
	 */

	amroot = getuid () == 0;
#ifdef	NDBM
#ifdef	SHADOWPWD
	sp_dbm_mode = O_RDWR;
#endif
	pw_dbm_mode = O_RDWR;
#endif

	/*
	 * Get the program name.  The program name is used as a
	 * prefix to most error messages.  It is also used as input
	 * to the openlog() function for error logging.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif

	/*
	 * Start with the flags which cause another command to be
	 * executed.  The effective UID will be set back to the
	 * real UID and the new command executed with the flags
	 */

	if (argc > 1 && argv[1][0] == '-' && strchr ("gfs", argv[1][1])) {
		setuid (getuid ());
		switch (argv[1][1]) {
			case 'g':
				argv[1] = "gpasswd";
				execv ("/bin/gpasswd", &argv[1]);
				break;
			case 'f':
				argv[1] = "chfn";
				execv ("/bin/chfn", &argv[1]);
				break;
			case 's':
				argv[1] = "chsh";
				execv ("/bin/chsh", &argv[1]);
				break;
			default:
				usage ();
		}
		sprintf (buf, EXECFAILED, Prog, argv[1]);
		perror (buf);
#ifdef	USE_SYSLOG
		syslog (LOG_CRIT, EXECFAILED2, argv[1]);
		closelog ();
#endif
		exit (1);
	}

	/* 
	 * The remaining arguments will be processed one by one and
	 * executed by this command.  The name is the last argument
	 * if it does not begin with a "-", otherwise the name is
	 * determined from the environment and must agree with the
	 * real UID.  Also, the UID will be checked for any commands
	 * which are restricted to root only.
	 */

#ifdef	SHADOWPWD
	while ((flag = getopt (argc, argv, "ludx:n:w:i:S")) != EOF)
#else
	while ((flag = getopt (argc, argv, "ludx:n:S")) != EOF)
#endif
	{
		switch (flag) {
			case 'x':
				max = strtol (optarg, &cp, 10);
#ifndef	SHADOWPWD
				if (max > 0)
					max /= 7;
#endif
				if (*cp || getuid ())
					usage ();

				xflg++;
				break;
			case 'n':
				min = strtol (optarg, &cp, 10);
#ifndef	SHADOWPWD
				if (min > 0)
					min /= 7;
#endif
				if (*cp || getuid ())
					usage ();

				nflg++;
				break;
#ifdef	SHADOWPWD
			case 'w':
				warn = strtol (optarg, &cp, 10);
				if (*cp || getuid ())
					usage ();

				if (warn >= -1)
					wflg++;

				break;
			case 'i':
				inact = strtol (optarg, &cp, 10);
				if (*cp || getuid ())
					usage ();

				if (inact >= -1)
					iflg++;

				break;
#endif	/* SHADOWPWD */
			case 'S':
				if (getuid ())
					usage ();

				Sflg++;
				break;
			case 'd':
				if (getuid ())
					usage ();

				dflg++;
				break;
			case 'l':
				if (getuid ())
					usage ();

				lflg++;
				break;
			case 'u':
				if (getuid ())
					usage ();

				uflg++;
				break;
			default:
				usage ();
		}
	}

	/*
	 * If any of the flags were given, a user name must be supplied
	 * on the command line.  Only an unadorned command line doesn't
	 * require the user's name be given.  Also, on -x, -n, -m, and
	 * -i may appear with each other.  -d, -l and -S must appear alone.
	 */

#ifdef	SHADOWPWD
	if ((dflg || lflg || xflg || nflg ||
				wflg || iflg || Sflg) && optind >= argc)
#else
	if ((dflg || lflg || xflg || nflg || Sflg) && optind >= argc)
#endif
		usage ();

#ifdef	SHADOWPWD
	if ((dflg + lflg + uflg + (xflg || nflg || wflg || iflg) + Sflg) > 1)
#else
	if ((dflg + lflg + uflg + (xflg || nflg) + Sflg) > 1)
#endif
		usage ();

	/*
	 * Now I have to get the user name.  The name will be gotten 
	 * from the command line if possible.  Otherwise it is figured
	 * out from the environment.
	 */

	if (optind < argc) {
		strncpy (name, argv[optind], sizeof name);
		name[sizeof name - 1] = '\0';
	} else if (amroot) {
		strcpy (name, "root");
	} else if (cp = getlogin ()) {
		strncpy (name, cp, sizeof name);
		name[sizeof name - 1] = '\0';
	} else {
		fprintf (stderr, WHOAREYOU, Prog);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Now I have a name, let's see if the UID for the name
	 * matches the current real UID.
	 */

	if (! (pw = getpwnam (name))) {
		fprintf (stderr, UNKUSER, Prog, name);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (! amroot && pw->pw_uid != getuid ()) {
		fprintf (stderr, NOPERM, name);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, NOPERM2, name);
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Let the user know whose password is being changed.
	 */

	if (! Sflg)
		printf (CHANGING, name);

#ifdef	SHADOWPWD
	/*
	 * The user name is valid, so let's get the shadow file
	 * entry.
	 */

	if (! (sp = getspnam (name)))
		pwd_to_spwd (pw, sp = &tspwd);

	/*
	 * Save the shadow entry off to the side so it doesn't
	 * get changed by any of the following code.
	 */

	if (sp != &tspwd) {
		tspwd = *sp;
		sp = &tspwd;
	}
	tspwd.sp_namp = strdup (sp->sp_namp);
	tspwd.sp_pwdp = strdup (sp->sp_pwdp);
#endif	/* SHADOWPWD */

	if (Sflg) {
#ifdef	SHADOWPWD
		print_status (pw, sp);
#else
		print_status (pw);
#endif
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (0);
	}

	/*
	 * If there are no other flags, just change the password.
	 */

#ifdef	SHADOWPWD
	if (! (dflg || lflg || uflg || xflg || nflg || wflg || iflg))
#else
	if (! (dflg || lflg || uflg || xflg || nflg))
#endif
	{
#ifdef	SHADOWPWD
		if (strchr (sp->sp_pwdp, '@')) {
			if (pw_auth (sp->sp_pwdp, name, PW_CHANGE, 0)) {
#ifdef	USE_SYSLOG
				syslog (LOG_INFO, NOCHGPASSWD, name);
				closelog ();
#endif
				exit (1);
			} else {
				update_age (sp);

				if (! uses_default_method (sp->sp_pwdp))
					goto done;
			}
		} else
			strcpy (crypt_passwd, sp->sp_pwdp);
#else	/* !SHADOWPWD */
		if (strchr (pw->pw_passwd, '@')) {
			if (pw_auth (pw->pw_passwd, name, PW_CHANGE, 0)) {
#ifdef	USE_SYSLOG
				syslog (LOG_INFO, CHGPASSWD, name);
				closelog ();
#endif
				exit (0);
			} else {
				update_age (pw);

				if (! uses_default_method (pw->pw_passwd))
					goto done;
			}
		} else
			strcpy (crypt_passwd, pw_pw_passwd);
#endif	/* SHADOWPWD */

		/*
		 * See if the user is permitted to change the password.
		 * Otherwise, go ahead and set a new password.
		 */

#ifdef	SHADOWPWD
		check_password (pw, sp);
		if (new_password (pw, sp))
#else
#ifdef	AGING

		/*
		 * Only check the age when there is one to check.
		 */

		check_password (pw);
#endif
		if (new_password (pw))
#endif
		{
			fprintf (stderr, UNCHANGED, name);
#ifdef	USE_SYSLOG
			closelog ();
#endif
			exit (1);
		}
	}

	/*
	 * The other options are incredibly simple.  Just modify the
	 * field in the shadow file entry.
	 */

	if (dflg)			/* Set password to blank */
#ifdef	SHADOWPWD
		sp->sp_pwdp = "";
#else
		pw->pw_passwd = "";
#endif
	if (lflg) {			/* Set password to "locked" value */
#ifdef	SHADOWPWD
		if (sp->sp_pwdp && sp->sp_pwdp[0] != '!') {
			strcpy (new_passwd, "!");
			strcat (new_passwd, sp->sp_pwdp);
			sp->sp_pwdp = new_passwd;
		}
#else
		if (pw->pw_passwd & pw->pw_passwd[0] != '!') {
			strcpy (new_passwd, "!");
			strcat (new_passwd, pw->pw_passwd);
			pw->pw_passwd = new_passwd;
		}
#endif
	}
	if (uflg) {			/* Undo password "locked" value */
#ifdef	SHADOWPWD
		if (sp->sp_pwdp && sp->sp_pwdp[0] == '!') {
			strcpy (new_passwd, sp->sp_pwdp + 1);
			sp->sp_pwdp = new_passwd;
		}
#else
		if (pw->pw_passwd && pw->pw_passwd[0] == '!') {
			strcpy (new_passwd, pw->pw_passwd + 1);
			pw->pw_passwd = new_passwd;
		}
#endif
	}
#if !defined(SHADOWPWD) && defined(ATT_AGE)
	bzero (age, sizeof age);
	strcpy (age, pw->pw_age);
#endif
	if (xflg) {
#ifdef	SHADOWPWD
		sp->sp_max = (max * DAY) / SCALE;
#else
		age[0] = i64c (max);
#endif
	}
	if (nflg) {
#ifdef	SHADOWPWD
		sp->sp_min = (min * DAY) / SCALE;
#else
		if (age[0] == '\0')
			age[0] = '/';

		age[1] = i64c (min);
#endif
	}
#ifdef	SHADOWPWD
	if (wflg)
		sp->sp_warn = (warn * DAY) / SCALE;

	if (iflg)
		sp->sp_inact = (inact * DAY) / SCALE;
#endif
#if !defined(SHADOWPWD) && defined(ATT_AGE)
	pw->pw_age = age;
#endif

done:
	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals.  Any
	 * keyboard signals are set to be ignored.
	 */

#ifdef	HAVE_ULIMIT
	ulimit (UL_SFILLIM, 30000);
#endif
#ifdef	HAVE_RLIMIT
	setrlimit (RLIMIT_FSIZE, &rlimit_fsize);
#endif
	if (setuid (0)) {
		fprintf (stderr, NOTROOT);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, NOTROOT2);
		closelog ();
#endif
		exit (1);
	}
	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif

	/*
	 * The shadow entry is now ready to be committed back to
	 * the shadow file.  Get a lock on the file and open it.
	 */

	for (i = 0;i < 30;i++)
#ifdef	SHADOWPWD
		if (spw_lock ())
#else
		if (pw_lock ())
#endif
			break;

	if (i == 30) {
#ifdef	SHADOWPWD
		fprintf (stderr, SPWDBUSY);
#else
		fprintf (stderr, PWDBUSY);
#endif
#ifdef	USE_SYSLOG
#ifdef	SHADOWPWD
		syslog (LOG_WARN, SPWDBUSY2);
#else
		syslog (LOG_WARN, PWDBUSY2);
#endif
		closelog ();
#endif
		exit (1);
	}
#ifdef	SHADOWPWD
	if (! spw_open (O_RDWR))
#else
	if (! pw_open (O_RDWR))
#endif
	{
		fprintf (stderr, OPNERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, OPNERROR2);
		closelog ();
#endif
#ifdef	SHADOWPWD
		(void) spw_unlock ();
#else
		(void) pw_unlock ();
#endif
		exit (1);
	}

	/*
	 * Update the shadow file entry.  If there is a DBM file,
	 * update that entry as well.
	 */

#ifdef	SHADOWPWD
	if (! spw_update (sp))
#else
	if (! pw_update (pw))
#endif
	{
		fprintf (stderr, UPDERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, UPDERROR2);
		closelog ();
#endif
#ifdef	SHADOWPWD
		(void) spw_unlock ();
#else
		(void) pw_unlock ();
#endif
		exit (1);
	}
#ifdef	NDBM
#ifdef	SHADOWPWD
	if (access ("/etc/shadow.pag", 0) == 0 && ! sp_dbm_update (sp))
#else
	if (access ("/etc/passwd.pag", 0) == 0 && ! pw_dbm_update (pw))
#endif
	{
		fprintf (stderr, DBMERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, DBMERROR2);
		closelog ();
#endif
#ifdef	SHADOWPWD
		(void) spw_unlock ();
#else
		(void) pw_unlock ();
#endif
		exit (1);
	}
#ifdef	SHADOWPWD
	endspent ();
#endif
	endpwent ();
#endif	/* NDBM */

	/*
	 * Changes have all been made, so commit them and unlock the
	 * file.
	 */

#ifdef	SHADOWPWD
	if (! spw_close ())
#else
	if (! pw_close ())
#endif
	{
		fprintf (stderr, CLSERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, CLSERROR2);
		closelog ();
#endif
#ifdef	SHADOWPWD
		(void) spw_unlock ();
#else
		(void) pw_unlock ();
#endif
		exit (1);
	}
#ifdef	SHADOWPWD
	if (! spw_unlock ())
#else
	if (! pw_unlock ())
#endif
	{
		fprintf (stderr, UNLKERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, UNLKERROR2);
		closelog ();
#endif
		exit (1);
	}
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, CHGPASSWD, name);
	closelog ();
#endif
	exit (0);
	/*NOTREACHED*/
}
