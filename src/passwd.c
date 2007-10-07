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

#include "rcsid.h"
RCSID (PKG_VER "$Id: passwd.c,v 1.41 2005/07/07 15:32:50 kloczek Exp $")
#include "prototypes.h"
#include "defines.h"
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#ifndef GPASSWD_PROGRAM
#define GPASSWD_PROGRAM "gpasswd"
#endif
#ifndef CHFN_PROGRAM
#define CHFN_PROGRAM "chfn"
#endif
#ifndef CHSH_PROGRAM
#define CHSH_PROGRAM "chsh"
#endif
#include <pwd.h>
#include "pwauth.h"
#include "shadowio.h"
#include "pwio.h"
#include "nscd.h"
#include "getdef.h"
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#include <selinux/av_permissions.h>
#endif
/*
 * exit status values
 */
#define E_SUCCESS	0	/* success */
#define E_NOPERM	1	/* permission denied */
#define E_USAGE		2	/* invalid combination of options */
#define E_FAILURE	3	/* unexpected failure, nothing done */
#define E_MISSING	4	/* unexpected failure, passwd file missing */
#define E_PWDBUSY	5	/* passwd file busy, try again later */
#define E_BAD_ARG	6	/* invalid argument to option */
/*
 * Global variables
 */
static char *name;		/* The name of user whose password is being changed */
static char *myname;		/* The current user's name */
static char *Prog;		/* Program name */
static int amroot;		/* The real UID was 0 */

static int
 eflg = 0,			/* -e - force password change */
    iflg = 0,			/* -i - set inactive days */
    kflg = 0,			/* -k - change only if expired */
    nflg = 0,			/* -n - set minimum days */
    wflg = 0,			/* -w - set warning days */
    xflg = 0,			/* -x - set maximum days */
    aflg = 0,			/* -a - show status for all users */
    dflg = 0,			/* -d - delete password */
    lflg = 0,			/* -l - lock account */
    qflg = 0,			/* -q - quiet mode */
    Sflg = 0,			/* -S - show password status */
    uflg = 0;			/* -u - unlock account */

/*
 * set to 1 if there are any flags which require root privileges,
 * and require username to be specified
 */
static int anyflag = 0;

static long age_min = 0;	/* Minimum days before change   */
static long age_max = 0;	/* Maximum days until change     */
static long warn = 0;		/* Warning days before change   */
static long inact = 0;		/* Days without change before locked */

static int do_update_age = 0;

#ifndef USE_PAM
static char crypt_passwd[128];	/* The "old-style" password, if present */
static int do_update_pwd = 0;
#endif

/*
 * External identifiers
 */

/* local function prototypes */
static void usage (int);

#ifndef USE_PAM
static int reuse (const char *, const struct passwd *);
static int new_password (const struct passwd *);

static void check_password (const struct passwd *, const struct spwd *);
static char *insert_crypt_passwd (const char *, const char *);
#endif				/* !USE_PAM */
static char *date_to_str (time_t);
static const char *pw_status (const char *);
static void print_status (const struct passwd *);
static void fail_exit (int);
static void oom (void);
static char *update_crypt_pw (char *);
static void update_noshadow (void);

static void update_shadow (void);
static long getnumber (const char *);

/*
 * usage - print command usage and exit
 */

static void usage (int status)
{
	fprintf (stderr, _("Usage: %s [-f|-s] [name]\n"), Prog);
	if (amroot) {
		fprintf (stderr,
			 _
			 ("       %s [-x max] [-n min] [-w warn] [-i inact] name\n"),
			 Prog);
		fprintf (stderr, _("       %s {-l|-u|-d|-S|-e} name\n"), Prog);
	}
	exit (status);
}

#ifndef USE_PAM
static int reuse (const char *pass, const struct passwd *pw)
{
#ifdef HAVE_LIBCRACK_HIST
	const char *reason;

#ifdef HAVE_LIBCRACK_PW
	const char *FascistHistoryPw (const char *, const struct passwd *);

	reason = FascistHistory (pass, pw);
#else
	const char *FascistHistory (const char *, int);

	reason = FascistHistory (pass, pw->pw_uid);
#endif
	if (reason) {
		printf (_("Bad password: %s.  "), reason);
		return 1;
	}
#endif
	return 0;
}

/*
 * new_password - validate old password and replace with new (both old and
 * new in global "char crypt_passwd[128]")
 */

 /*ARGSUSED*/ static int new_password (const struct passwd *pw)
{
	char *clear;		/* Pointer to clear text */
	char *cipher;		/* Pointer to cipher text */
	char *cp;		/* Pointer to getpass() response */
	char orig[200];		/* Original password */
	char pass[200];		/* New password */
	int i;			/* Counter for retries */
	int warned;
	int pass_max_len;

#ifdef HAVE_LIBCRACK_HIST
	int HistUpdate (const char *, const char *);
#endif

	/*
	 * Authenticate the user. The user will be prompted for their own
	 * password.
	 */

	if (!amroot && crypt_passwd[0]) {
		if (!(clear = getpass (_("Old password: "))))
			return -1;

		cipher = pw_encrypt (clear, crypt_passwd);
		if (strcmp (cipher, crypt_passwd) != 0) {
			SYSLOG ((LOG_WARN, "incorrect password for `%s'",
				 pw->pw_name));
			sleep (1);
			fprintf (stderr,
				 _("Incorrect password for `%s'\n"),
				 pw->pw_name);
			return -1;
		}
		STRFCPY (orig, clear);
		strzero (clear);
		strzero (cipher);
	} else {
		orig[0] = '\0';
	}

	/*
	 * Get the new password. The user is prompted for the new password
	 * and has five tries to get it right. The password will be tested
	 * for strength, unless it is the root user. This provides an escape
	 * for initial login passwords.
	 */

	if (getdef_bool ("MD5_CRYPT_ENAB"))
		pass_max_len = 127;
	else
		pass_max_len = getdef_num ("PASS_MAX_LEN", 8);

	if (!qflg)
		printf (_("\
Enter the new password (minimum of %d, maximum of %d characters)\n\
Please use a combination of upper and lower case letters and numbers.\n"), getdef_num ("PASS_MIN_LEN", 5), pass_max_len);

	warned = 0;
	for (i = getdef_num ("PASS_CHANGE_TRIES", 5); i > 0; i--) {
		if (!(cp = getpass (_("New password: ")))) {
			memzero (orig, sizeof orig);
			return -1;
		}
		if (warned && strcmp (pass, cp) != 0)
			warned = 0;
		STRFCPY (pass, cp);
		strzero (cp);

		if (!amroot && (!obscure (orig, pass, pw) || reuse (pass, pw))) {
			printf (_("Try again.\n"));
			continue;
		}
		/*
		 * If enabled, warn about weak passwords even if you are
		 * root (enter this password again to use it anyway). 
		 * --marekm
		 */
		if (amroot && !warned && getdef_bool ("PASS_ALWAYS_WARN")
		    && (!obscure (orig, pass, pw) || reuse (pass, pw))) {
			printf (_
				("\nWarning: weak password (enter it again to use it anyway).\n"));
			warned++;
			continue;
		}
		if (!(cp = getpass (_("Re-enter new password: ")))) {
			memzero (orig, sizeof orig);
			return -1;
		}
		if (strcmp (cp, pass))
			fprintf (stderr, _("They don't match; try again.\n"));
		else {
			strzero (cp);
			break;
		}
	}
	memzero (orig, sizeof orig);

	if (i == 0) {
		memzero (pass, sizeof pass);
		return -1;
	}

	/*
	 * Encrypt the password, then wipe the cleartext password.
	 */

	cp = pw_encrypt (pass, crypt_make_salt ());
	memzero (pass, sizeof pass);

#ifdef HAVE_LIBCRACK_HIST
	HistUpdate (pw->pw_name, crypt_passwd);
#endif
	STRFCPY (crypt_passwd, cp);
	return 0;
}

/*
 * check_password - test a password to see if it can be changed
 *
 *	check_password() sees if the invoker has permission to change the
 *	password for the given user.
 */

static void check_password (const struct passwd *pw, const struct spwd *sp)
{
	time_t now, last, ok;
	int exp_status;

	exp_status = isexpired (pw, sp);

	/*
	 * If not expired and the "change only if expired" option (idea from
	 * PAM) was specified, do nothing. --marekm
	 */
	if (kflg && exp_status == 0)
		exit (E_SUCCESS);

	/*
	 * Root can change any password any time.
	 */

	if (amroot)
		return;

	time (&now);

	/*
	 * Expired accounts cannot be changed ever. Passwords which are
	 * locked may not be changed. Passwords where min > max may not be
	 * changed. Passwords which have been inactive too long cannot be
	 * changed.
	 */

	if (sp->sp_pwdp[0] == '!' || exp_status > 1 ||
	    (sp->sp_max >= 0 && sp->sp_min > sp->sp_max)) {
		fprintf (stderr,
			 _("The password for %s cannot be changed.\n"),
			 sp->sp_namp);
		SYSLOG ((LOG_WARN, "password locked for `%s'", sp->sp_namp));
		closelog ();
		exit (E_NOPERM);
	}

	/*
	 * Passwords may only be changed after sp_min time is up.
	 */

	last = sp->sp_lstchg * SCALE;
	ok = last + (sp->sp_min > 0 ? sp->sp_min * SCALE : 0);

	if (now < ok) {
		fprintf (stderr,
			 _
			 ("Sorry, the password for %s cannot be changed yet.\n"),
			 pw->pw_name);
		SYSLOG ((LOG_WARN, "now < minimum age for `%s'", pw->pw_name));
		closelog ();
		exit (E_NOPERM);
	}
}

/*
 * insert_crypt_passwd - add an "old-style" password to authentication
 * string result now malloced to avoid overflow, just in case.  --marekm
 */
static char *insert_crypt_passwd (const char *string, const char *passwd)
{
	return xstrdup (passwd);
}
#endif				/* !USE_PAM */

static char *date_to_str (time_t t)
{
	static char buf[80];
	struct tm *tm;

	tm = gmtime (&t);
#ifdef HAVE_STRFTIME
	strftime (buf, sizeof buf, "%m/%d/%Y", tm);
#else
	snprintf (buf, sizeof buf, "%02d/%02d/%04d",
		  tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);
#endif
	return buf;
}

static const char *pw_status (const char *pass)
{
	if (*pass == '*' || *pass == '!')
		return "L";
	if (*pass == '\0')
		return "NP";
	return "P";
}

/*
 * print_status - print current password status
 */

static void print_status (const struct passwd *pw)
{
	struct spwd *sp;

	sp = getspnam (pw->pw_name);
	if (sp) {
		printf ("%s %s %s %ld %ld %ld %ld\n",
			pw->pw_name,
			pw_status (sp->sp_pwdp),
			date_to_str (sp->sp_lstchg * SCALE),
			(sp->sp_min * SCALE) / DAY,
			(sp->sp_max * SCALE) / DAY,
			(sp->sp_warn * SCALE) / DAY,
			(sp->sp_inact * SCALE) / DAY);
	} else {
		printf ("%s %s\n", pw->pw_name, pw_status (pw->pw_passwd));
	}
}


static void fail_exit (int status)
{
	pw_unlock ();
	spw_unlock ();
	exit (status);
}

static void oom (void)
{
	fprintf (stderr, _("%s: out of memory\n"), Prog);
	fail_exit (E_FAILURE);
}

static char *update_crypt_pw (char *cp)
{
#ifndef USE_PAM
	if (do_update_pwd)
		cp = insert_crypt_passwd (cp, crypt_passwd);
#endif

	if (dflg)
		cp = "";	/* XXX warning: const */

	if (uflg && *cp == '!')
		cp++;

	if (lflg && *cp != '!') {
		char *newpw = xmalloc (strlen (cp) + 2);

		strcpy (newpw, "!");
		strcat (newpw, cp);
		cp = newpw;
	}
	return cp;
}


static void update_noshadow (void)
{
	const struct passwd *pw;
	struct passwd *npw;

	if (!pw_lock ()) {
		fprintf (stderr,
			 _
			 ("Cannot lock the password file; try again later.\n"));
		SYSLOG ((LOG_WARN, "can't lock password file"));
		exit (E_PWDBUSY);
	}
	if (!pw_open (O_RDWR)) {
		fprintf (stderr, _("Cannot open the password file.\n"));
		SYSLOG ((LOG_ERR, "can't open password file"));
		fail_exit (E_MISSING);
	}
	pw = pw_locate (name);
	if (!pw) {
		fprintf (stderr, _("%s: %s not found in /etc/passwd\n"),
			 Prog, name);
		fail_exit (E_NOPERM);
	}
	npw = __pw_dup (pw);
	if (!npw)
		oom ();
	npw->pw_passwd = update_crypt_pw (npw->pw_passwd);
	if (!pw_update (npw)) {
		fprintf (stderr, _("Error updating the password entry.\n"));
		SYSLOG ((LOG_ERR, "error updating password entry"));
		fail_exit (E_FAILURE);
	}
	if (!pw_close ()) {
		fprintf (stderr, _("Cannot commit password file changes.\n"));
		SYSLOG ((LOG_ERR, "can't rewrite password file"));
		fail_exit (E_FAILURE);
	}
	pw_unlock ();
}

static void update_shadow (void)
{
	const struct spwd *sp;
	struct spwd *nsp;

	if (!spw_lock ()) {
		fprintf (stderr,
			 _
			 ("Cannot lock the password file; try again later.\n"));
		SYSLOG ((LOG_WARN, "can't lock password file"));
		exit (E_PWDBUSY);
	}
	if (!spw_open (O_RDWR)) {
		fprintf (stderr, _("Cannot open the password file.\n"));
		SYSLOG ((LOG_ERR, "can't open password file"));
		fail_exit (E_FAILURE);
	}
	sp = spw_locate (name);
	if (!sp) {
		/* Try to update the password in /etc/passwd instead. */
		spw_close ();
		update_noshadow ();
		spw_unlock ();
		return;
	}
	nsp = __spw_dup (sp);
	if (!nsp)
		oom ();
	nsp->sp_pwdp = update_crypt_pw (nsp->sp_pwdp);
	if (xflg)
		nsp->sp_max = (age_max * DAY) / SCALE;
	if (nflg)
		nsp->sp_min = (age_min * DAY) / SCALE;
	if (wflg)
		nsp->sp_warn = (warn * DAY) / SCALE;
	if (iflg)
		nsp->sp_inact = (inact * DAY) / SCALE;
	if (do_update_age)
		nsp->sp_lstchg = time ((time_t *) 0) / SCALE;
	/*
	 * Force change on next login, like SunOS 4.x passwd -e or Solaris
	 * 2.x passwd -f. Solaris 2.x seems to do the same thing (set
	 * sp_lstchg to 0).
	 */
	if (eflg)
		nsp->sp_lstchg = 0;

	if (!spw_update (nsp)) {
		fprintf (stderr, _("Error updating the password entry.\n"));
		SYSLOG ((LOG_ERR, "error updating password entry"));
		fail_exit (E_FAILURE);
	}
	if (!spw_close ()) {
		fprintf (stderr, _("Cannot commit password file changes.\n"));
		SYSLOG ((LOG_ERR, "can't rewrite password file"));
		fail_exit (E_FAILURE);
	}
	spw_unlock ();
}

static long getnumber (const char *str)
{
	long val;
	char *cp;

	val = strtol (str, &cp, 10);
	if (*cp)
		usage (E_BAD_ARG);
	return val;
}

/*
 * passwd - change a user's password file information
 *
 *	This command controls the password file and commands which are used
 * 	to modify it.
 *
 *	The valid options are
 *
 *	-l	lock the named account (*)
 *	-u	unlock the named account (*)
 *	-d	delete the password for the named account (*)
 *	-e	expire the password for the named account (*)
 *	-x #	set sp_max to # days (*)
 *	-n #	set sp_min to # days (*)
 *	-w #	set sp_warn to # days (*)
 *	-i #	set sp_inact to # days (*)
 *	-S	show password status of named account
 *	-g	execute gpasswd command to interpret flags
 *	-f	execute chfn command to interpret flags
 *	-s	execute chsh command to interpret flags
 *	-k	change password only if expired
 *
 *	(*) requires root permission to execute.
 *
 *	All of the time fields are entered in days and converted to the
 * 	appropriate internal format. For finer resolute the chage
 *	command must be used.
 */

int main (int argc, char **argv)
{
	int flag;		/* Current option to process     */
	const struct passwd *pw;	/* Password file entry for user      */

#ifndef USE_PAM
	char *cp;		/* Miscellaneous character pointing  */

	const struct spwd *sp;	/* Shadow file entry for user   */
#endif

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	/*
	 * The program behaves differently when executed by root than when
	 * executed by a normal user.
	 */
	amroot = (getuid () == 0);

	/*
	 * Get the program name. The program name is used as a prefix to
	 * most error messages.
	 */
	Prog = Basename (argv[0]);

	sanitize_env ();

	OPENLOG ("passwd");

	/*
	 * Start with the flags which cause another command to be executed.
	 * The effective UID will be set back to the real UID and the new
	 * command executed with the flags
	 *
	 * These flags are deprecated, may change in a future release.
	 * Please run these programs directly.  --marekm
	 */

	if (argc > 1 && argv[1][0] == '-' && strchr ("gfs", argv[1][1])) {
		char buf[200];

		setuid (getuid ());
		switch (argv[1][1]) {
		case 'g':
			argv[1] = GPASSWD_PROGRAM;	/* XXX warning: const */
			break;
		case 'f':
			argv[1] = CHFN_PROGRAM;	/* XXX warning: const */
			break;
		case 's':
			argv[1] = CHSH_PROGRAM;	/* XXX warning: const */
			break;
		default:
			usage (E_BAD_ARG);
		}
		snprintf (buf, sizeof buf, _("%s: Cannot execute %s"),
			  Prog, argv[1]);
		execvp (argv[1], &argv[1]);
		perror (buf);
		SYSLOG ((LOG_ERR, "cannot execute %s", argv[1]));
		closelog ();
		exit (E_FAILURE);
	}

	/* 
	 * The remaining arguments will be processed one by one and executed
	 * by this command. The name is the last argument if it does not
	 * begin with a "-", otherwise the name is determined from the
	 * environment and must agree with the real UID. Also, the UID will
	 * be checked for any commands which are restricted to root only.
	 */


	while ((flag = getopt (argc, argv, "adei:kln:qr:Suw:x:")) != EOF) {
		switch (flag) {
		case 'a':
			aflg++;
			break;
		case 'd':
			dflg++;
			anyflag = 1;
			break;
		case 'e':
			eflg++;
			anyflag = 1;
			break;
		case 'i':
			inact = getnumber (optarg);
			if (inact >= -1)
				iflg++;
			anyflag = 1;
			break;
		case 'k':
			/* change only if expired, like Linux-PAM passwd -k. */
			kflg++;	/* ok for users */
			break;
		case 'l':
			lflg++;
			anyflag = 1;
			break;
		case 'n':
			age_min = getnumber (optarg);
			nflg++;
			anyflag = 1;
			break;
		case 'q':
			qflg++;	/* ok for users */
			break;
		case 'S':
			Sflg++;	/* ok for users */
			break;
		case 'u':
			uflg++;
			anyflag = 1;
			break;
		case 'w':
			warn = getnumber (optarg);
			if (warn >= -1)
				wflg++;
			anyflag = 1;
			break;
		case 'r':
			/* -r repository (files|nis|nisplus) */
			/* only "files" supported for now */
			if (strcmp (optarg, "files") != 0) {
				fprintf (stderr,
					 _
					 ("%s: repository %s not supported\n"),
					 Prog, optarg);
				exit (E_BAD_ARG);
			}
			break;
		case 'x':
			age_max = getnumber (optarg);
			xflg++;
			anyflag = 1;
			break;
		default:
			usage (E_BAD_ARG);
		}
	}

	/*
	 * Now I have to get the user name. The name will be gotten from the
	 * command line if possible. Otherwise it is figured out from the
	 * environment.
	 */

	pw = get_my_pwent ();
	if (!pw) {
		fprintf (stderr,
			 _("%s: Cannot determine your user name.\n"), Prog);
		exit (E_NOPERM);
	}
	myname = xstrdup (pw->pw_name);
	if (optind < argc)
		name = argv[optind];
	else
		name = myname;

	/*
	 * The -a flag requires -S, no other flags, no username, and
	 * you must be root.  --marekm
	 */

	if (aflg) {
		if (anyflag || !Sflg || (optind < argc))
			usage (E_USAGE);
		if (!amroot) {
			fprintf (stderr, _("%s: Permission denied.\n"), Prog);
			exit (E_NOPERM);
		}
		setpwent ();
		while ((pw = getpwent ()))
			print_status (pw);
		exit (E_SUCCESS);
	}
#if 0
	/*
	 * Allow certain users (administrators) to change passwords of
	 * certain users. Not implemented yet. --marekm
	 */
	if (may_change_passwd (myname, name))
		amroot = 1;
#endif

	/*
	 * If any of the flags were given, a user name must be supplied on
	 * the command line. Only an unadorned command line doesn't require
	 * the user's name be given. Also, -x, -n, -w, -i, -e, -d,
	 * -l, -u may appear with each other. -S, -k must appear alone.
	 */

	/*
	 * -S now ok for normal users (check status of my own account), and
	 * doesn't require username.  --marekm
	 */

	if (anyflag && optind >= argc)
		usage (E_USAGE);

	if (anyflag + Sflg + kflg > 1)
		usage (E_USAGE);

	if (anyflag && !amroot) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
		exit (E_NOPERM);
	}

	pw = getpwnam (name);
	if (!pw) {
		fprintf (stderr, _("%s: unknown user %s\n"), Prog, name);
		exit (E_NOPERM);
	}
#ifdef WITH_SELINUX
	/*
	 * If the UID of the user does not match the current real UID,
	 * check if the change is allowed by SELinux policy.
	 */
	if ((pw->pw_uid != getuid ())
	    && (checkPasswdAccess (PASSWD__PASSWD) != 0)) {
#else
	/*
	 * If the UID of the user does not match the current real UID,
	 * check if I'm root.
	 */
	if (!amroot && pw->pw_uid != getuid ()) {
#endif
		fprintf (stderr,
			 _
			 ("%s: You may not view or modify password information for %s.\n"),
			 Prog, name);
		SYSLOG ((LOG_WARN,
			 "%s: can't view or modify password information for %s",
			 Prog, name));
		closelog ();
		exit (E_NOPERM);
	}

	if (Sflg) {
		print_status (pw);
		exit (E_SUCCESS);
	}
#ifndef USE_PAM
	/*
	 * The user name is valid, so let's get the shadow file entry.
	 */

	sp = getspnam (name);
	if (!sp)
		sp = pwd_to_spwd (pw);

	cp = sp->sp_pwdp;

	/*
	 * If there are no other flags, just change the password.
	 */

	if (!anyflag) {
		STRFCPY (crypt_passwd, cp);

		/*
		 * See if the user is permitted to change the password. 
		 * Otherwise, go ahead and set a new password.
		 */

		check_password (pw, sp);

		/*
		 * Let the user know whose password is being changed.
		 */
		if (!qflg)
			printf (_("Changing password for %s\n"), name);

		if (new_password (pw)) {
			fprintf (stderr,
				 _("The password for %s is unchanged.\n"),
				 name);
			closelog ();
			exit (E_NOPERM);
		}
		do_update_pwd = 1;
		do_update_age = 1;
	}
#endif				/* !USE_PAM */
	/*
	 * Before going any further, raise the ulimit to prevent colliding
	 * into a lowered ulimit, and set the real UID to root to protect
	 * against unexpected signals. Any keyboard signals are set to be
	 * ignored.
	 */

	pwd_init ();

	/*
	 * Don't set the real UID for PAM...
	 */
#ifdef USE_PAM
	if (!anyflag) {
		do_pam_passwd (name, qflg, kflg);
		exit (E_SUCCESS);
	}
#endif				/* USE_PAM */
	if (setuid (0)) {
		fprintf (stderr, _("Cannot change ID to root.\n"));
		SYSLOG ((LOG_ERR, "can't setuid(0)"));
		closelog ();
		exit (E_NOPERM);
	}
	if (spw_file_present ())
		update_shadow ();
	else
		update_noshadow ();

	nscd_flush_cache ("passwd");
	nscd_flush_cache ("group");
	nscd_flush_cache ("shadow");

	SYSLOG ((LOG_INFO, "password for `%s' changed by `%s'", name, myname));
	closelog ();
	if (!qflg)
		printf (_("Password changed.\n"));
	exit (E_SUCCESS);
	/* NOT REACHED */
}
