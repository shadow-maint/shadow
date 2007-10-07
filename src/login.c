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

#ident "$Id: login.c,v 1.83 2006/03/07 15:47:32 kloczek Exp $"

#include <errno.h>
#include <grp.h>
#include <lastlog.h>
#ifdef UT_ADDR
#include <netdb.h>
#endif
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include "defines.h"
#include "faillog.h"
#include "failure.h"
#include "getdef.h"
#include "prototypes.h"
#include "pwauth.h"
#include "exitcodes.h"
#ifdef USE_PAM
#include "pam_defs.h"

static pam_handle_t *pamh = NULL;

#define PAM_FAIL_CHECK if (retcode != PAM_SUCCESS) { \
	fprintf(stderr,"\n%s\n",pam_strerror(pamh, retcode)); \
	SYSLOG((LOG_ERR,"%s",pam_strerror(pamh, retcode))); \
	pam_end(pamh, retcode); exit(1); \
   }
#define PAM_END { retcode = pam_close_session(pamh,0); \
		pam_end(pamh,retcode); }

#endif				/* USE_PAM */

/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif

/*
 * Global variables
 */
const char *hostname = "";

static struct passwd pwent;

#if HAVE_UTMPX_H
extern struct utmpx utxent;
struct utmpx failent;
#else
struct utmp failent;
#endif
extern struct utmp utent;

struct lastlog lastlog;
static int pflg = 0;
static int fflg = 0;

#ifdef RLOGIN
static int rflg = 0;
#else
#define rflg 0
#endif
static int hflg = 0;
static int preauth_flag = 0;

/*
 * Global variables.
 */

static char *Prog;
static int amroot;
static int timeout;

/*
 * External identifiers.
 */

extern char **newenvp;
extern size_t newenvc;

extern void dolastlog (struct lastlog *, const struct passwd *,
		       const char *, const char *);

extern char **environ;

#ifndef	ALARM
#define	ALARM	60
#endif

#ifndef	RETRIES
#define	RETRIES	3
#endif

/* local function prototypes */
static void usage (void);
static void setup_tty (void);
static void check_flags (int, char *const *);

#ifndef USE_PAM
extern int login_access (const char *, const char *);

static struct faillog faillog;

static void bad_time_notify (void);
static void check_nologin (void);
#endif

static void init_env (void);
static RETSIGTYPE alarm_handler (int);

/*
 * usage - print login command usage and exit
 *
 * login [ name ]
 * login -r hostname	(for rlogind)
 * login -h hostname	(for telnetd, etc.)
 * login -f name	(for pre-authenticated login: datakit, xterm, etc.)
 */
static void usage (void)
{
	fprintf (stderr, _("Usage: %s [-p] [name]\n"), Prog);
	if (!amroot)
		exit (1);
	fprintf (stderr, _("       %s [-p] [-h host] [-f name]\n"), Prog);
#ifdef RLOGIN
	fprintf (stderr, _("       %s [-p] -r host\n"), Prog);
#endif
	exit (1);
}

static void setup_tty (void)
{
	TERMIO termio;

	GTTY (0, &termio);	/* get terminal characteristics */

	/*
	 * Add your favorite terminal modes here ...
	 */
	termio.c_lflag |= ISIG | ICANON | ECHO | ECHOE;
	termio.c_iflag |= ICRNL;

	/* leave these values unchanged if not specified in login.defs */
	termio.c_cc[VERASE] = getdef_num ("ERASECHAR", termio.c_cc[VERASE]);
	termio.c_cc[VKILL] = getdef_num ("KILLCHAR", termio.c_cc[VKILL]);

	/*
	 * ttymon invocation prefers this, but these settings won't come into
	 * effect after the first username login 
	 */
	STTY (0, &termio);
}


#ifndef USE_PAM
/*
 * Tell the user that this is not the right time to login at this tty
 */
static void bad_time_notify (void)
{
	printf (_("Invalid login time\n"));
	fflush (stdout);
}

static void check_nologin (void)
{
	char *fname;

	/*
	 * Check to see if system is turned off for non-root users.
	 * This would be useful to prevent users from logging in
	 * during system maintenance. We make sure the message comes
	 * out for root so she knows to remove the file if she's
	 * forgotten about it ...
	 */
	fname = getdef_str ("NOLOGINS_FILE");
	if (fname != NULL && access (fname, F_OK) == 0) {
		FILE *nlfp;
		int c;

		/*
		 * Cat the file if it can be opened, otherwise just
		 * print a default message
		 */
		if ((nlfp = fopen (fname, "r"))) {
			while ((c = getc (nlfp)) != EOF) {
				if (c == '\n')
					putchar ('\r');

				putchar (c);
			}
			fflush (stdout);
			fclose (nlfp);
		} else
			printf (_("\nSystem closed for routine maintenance\n"));
		/*
		 * Non-root users must exit. Root gets the message, but
		 * gets to login.
		 */

		if (pwent.pw_uid != 0) {
			closelog ();
			exit (0);
		}
		printf (_("\n[Disconnect bypassed -- root login allowed.]\n"));
	}
}
#endif				/* !USE_PAM */

static void check_flags (int argc, char *const *argv)
{
	int arg;

	/*
	 * Check the flags for proper form. Every argument starting with
	 * "-" must be exactly two characters long. This closes all the
	 * clever rlogin, telnet, and getty holes.
	 */
	for (arg = 1; arg < argc; arg++) {
		if (argv[arg][0] == '-' && strlen (argv[arg]) > 2)
			usage ();
	}
}


static void init_env (void)
{
#ifndef USE_PAM
	char *cp;
#endif
	char *tmp;

	if ((tmp = getenv ("LANG"))) {
		addenv ("LANG", tmp);
	}

	/*
	 * Add the timezone environmental variable so that time functions
	 * work correctly.
	 */
	if ((tmp = getenv ("TZ"))) {
		addenv ("TZ", tmp);
	}
#ifndef USE_PAM
	else if ((cp = getdef_str ("ENV_TZ")))
		addenv (*cp == '/' ? tz (cp) : cp, NULL);
#endif				/* !USE_PAM */
	/* 
	 * Add the clock frequency so that profiling commands work
	 * correctly.
	 */
	if ((tmp = getenv ("HZ"))) {
		addenv ("HZ", tmp);
	}
#ifndef USE_PAM
	else if ((cp = getdef_str ("ENV_HZ")))
		addenv (cp, NULL);
#endif				/* !USE_PAM */
}


static RETSIGTYPE alarm_handler (int sig)
{
	fprintf (stderr, _("\nLogin timed out after %d seconds.\n"), timeout);
	exit (0);
}


/*
 * login - create a new login session for a user
 *
 *	login is typically called by getty as the second step of a
 *	new user session. getty is responsible for setting the line
 *	characteristics to a reasonable set of values and getting
 *	the name of the user to be logged in. login may also be
 *	called to create a new user session on a pty for a variety
 *	of reasons, such as X servers or network logins.
 *
 *	the flags which login supports are
 *	
 *	-p - preserve the environment
 *	-r - perform autologin protocol for rlogin
 *	-f - do not perform authentication, user is preauthenticated
 *	-h - the name of the remote host
 */
int main (int argc, char **argv)
{
	char username[32];
	char tty[BUFSIZ];

#ifdef RLOGIN
	char term[128] = "";
#endif
#if defined(HAVE_STRFTIME) && !defined(USE_PAM)
	char ptime[80];
#endif
	int reason = PW_LOGIN;
	int delay;
	int retries;
	int failed;
	int flag;
	int subroot = 0;
	int is_console;
	int err;
	const char *cp;
	char *tmp;
	char fromhost[512];
	struct passwd *pwd;
	char **envp = environ;
	static char temp_pw[2];
	static char temp_shell[] = "/bin/sh";

#ifdef USE_PAM
	int retcode;
	pid_t child;
	char *pam_user;
#else
	struct spwd *spwd = NULL;
#endif
	/*
	 * Some quick initialization.
	 */

	sanitize_env ();

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	initenv ();

	username[0] = '\0';
	amroot = (getuid () == 0);
	Prog = Basename (argv[0]);

	check_flags (argc, argv);

	while ((flag = getopt (argc, argv, "d:f::h:pr:")) != EOF) {
		switch (flag) {
		case 'd':
			/* "-d device" ignored for compatibility */
			break;
		case 'f':
			/*
			 * username must be a separate token
			 * (-f root, *not* -froot).  --marekm
			 *
			 * if -f has an arg, use that, else use the
			 * normal user name passed after all options
			 * --benc
			 */
			if (optarg != NULL && optarg != argv[optind - 1])
				usage ();
			fflg++;
			if (optarg)
				STRFCPY (username, optarg);
			break;
		case 'h':
			hflg++;
			hostname = optarg;
			reason = PW_TELNET;
			break;
#ifdef	RLOGIN
		case 'r':
			rflg++;
			hostname = optarg;
			reason = PW_RLOGIN;
			break;
#endif
		case 'p':
			pflg++;
			break;
		default:
			usage ();
		}
	}

#ifdef RLOGIN
	/*
	 * Neither -h nor -f should be combined with -r.
	 */

	if (rflg && (hflg || fflg))
		usage ();
#endif

	/*
	 * Allow authentication bypass only if real UID is zero.
	 */

	if ((rflg || fflg || hflg) && !amroot) {
		fprintf (stderr, _("%s: Permission denied.\n"), Prog);
		exit (1);
	}

	if (!isatty (0) || !isatty (1) || !isatty (2))
		exit (1);	/* must be a terminal */

	/*
	 * Be picky if run by normal users (possible if installed setuid
	 * root), but not if run by root. This way it still allows logins
	 * even if your getty is broken, or if something corrupts utmp,
	 * but users must "exec login" which will use the existing utmp
	 * entry (will not overwrite remote hostname).  --marekm
	 */
	checkutmp (!amroot);
	STRFCPY (tty, utent.ut_line);
	is_console = console (tty);

	if (rflg || hflg) {
#ifdef UT_ADDR
		struct hostent *he;

		/*
		 * Fill in the ut_addr field (remote login IP address). XXX
		 * - login from util-linux does it, but this is not the
		 * right place to do it. The program that starts login
		 * (telnetd, rlogind) knows the IP address, so it should
		 * create the utmp entry and fill in ut_addr. 
		 * gethostbyname() is not 100% reliable (the remote host may
		 * be unknown, etc.).  --marekm
		 */
		if ((he = gethostbyname (hostname))) {
			utent.ut_addr = *((int32_t *) (he->h_addr_list[0]));
#endif
#ifdef UT_HOST
			strncpy (utent.ut_host, hostname,
				 sizeof (utent.ut_host));
#endif
#if HAVE_UTMPX_H
			strncpy (utxent.ut_host, hostname,
				 sizeof (utxent.ut_host));
#endif
			/*
			 * Add remote hostname to the environment. I think
			 * (not sure) I saw it once on Irix.  --marekm
			 */
			addenv ("REMOTEHOST", hostname);
		}
#ifdef __linux__
		/*
		 * workaround for init/getty leaving junk in ut_host at least in
		 * some version of RedHat.  --marekm
		 */
		else if (amroot)
			memzero (utent.ut_host, sizeof utent.ut_host);
#endif
		if (fflg)
			preauth_flag++;
		if (hflg)
			reason = PW_RLOGIN;
#ifdef RLOGIN
		if (rflg
		    && do_rlogin (hostname, username, sizeof username,
				  term, sizeof term))
			preauth_flag++;
#endif

		OPENLOG ("login");

		setup_tty ();

#ifndef USE_PAM
		umask (getdef_num ("UMASK", GETDEF_DEFAULT_UMASK));

		{
			/* 
			 * Use the ULIMIT in the login.defs file, and if
			 * there isn't one, use the default value. The
			 * user may have one for themselves, but otherwise,
			 * just take what you get.
			 */
			long limit = getdef_long ("ULIMIT", -1L);

			if (limit != -1)
				set_filesize_limit (limit);
		}

#endif
		/*
		 * The entire environment will be preserved if the -p flag
		 * is used.
		 */
		if (pflg)
			while (*envp)	/* add inherited environment, */
				addenv (*envp++, NULL);	/* some variables change later */

#ifdef RLOGIN
		if (term[0] != '\0')
			addenv ("TERM", term);
		else
#endif
			/* preserve TERM from getty */
		if (!pflg && (tmp = getenv ("TERM")))
			addenv ("TERM", tmp);

		init_env ();

		if (optind < argc) {	/* get the user name */
			if (rflg || (fflg && username[0]))
				usage ();

			STRFCPY (username, argv[optind]);
			strzero (argv[optind]);
			++optind;
		}
		if (optind < argc)	/* now set command line variables */
			set_env (argc - optind, &argv[optind]);

		if (rflg || hflg)
			cp = hostname;
		else
#ifdef	UT_HOST
		if (utent.ut_host[0])
			cp = utent.ut_host;
		else
#endif
#if HAVE_UTMPX_H
		if (utxent.ut_host[0])
			cp = utxent.ut_host;
		else
#endif
			cp = "";

		if (*cp)
			snprintf (fromhost, sizeof fromhost,
				  _(" on `%.100s' from `%.200s'"), tty, cp);
		else
			snprintf (fromhost, sizeof fromhost,
				  _(" on `%.100s'"), tty);

	      top:
		/* only allow ALARM sec. for login */
		signal (SIGALRM, alarm_handler);
		timeout = getdef_num ("LOGIN_TIMEOUT", ALARM);
		if (timeout > 0)
			alarm (timeout);

		environ = newenvp;	/* make new environment active */
		delay = getdef_num ("FAIL_DELAY", 1);
		retries = getdef_num ("LOGIN_RETRIES", RETRIES);

#ifdef USE_PAM
		retcode = pam_start ("login", username, &conv, &pamh);
		if (retcode != PAM_SUCCESS) {
			fprintf (stderr,
				 _("login: PAM Failure, aborting: %s\n"),
				 pam_strerror (pamh, retcode));
			SYSLOG ((LOG_ERR, "Couldn't initialize PAM: %s",
				 pam_strerror (pamh, retcode)));
			exit (99);
		}

		/*
		 * hostname & tty are either set to NULL or their correct values,
		 * depending on how much we know. We also set PAM's fail delay to
		 * ours.
		 */
		retcode = pam_set_item (pamh, PAM_RHOST, hostname);
		PAM_FAIL_CHECK;
		retcode = pam_set_item (pamh, PAM_TTY, tty);
		PAM_FAIL_CHECK;
#ifdef HAVE_PAM_FAIL_DELAY
		retcode = pam_fail_delay (pamh, 1000000 * delay);
		PAM_FAIL_CHECK;
#endif
		/* if fflg == 1, then the user has already been authenticated */
		if (!fflg || (getuid () != 0)) {
			int failcount = 0;
			char hostn[256];
			char login_prompt[256];	/* That's one hell of a prompt :) */

			/* Make the login prompt look like we want it */
			if (!gethostname (hostn, sizeof (hostn)))
				snprintf (login_prompt,
					  sizeof (login_prompt),
					  _("%s login: "), hostn);
			else
				snprintf (login_prompt,
					  sizeof (login_prompt), _("login: "));

			retcode =
			    pam_set_item (pamh, PAM_USER_PROMPT, login_prompt);
			PAM_FAIL_CHECK;

			/* if we didn't get a user on the command line,
			   set it to NULL */
			pam_get_item (pamh, PAM_USER,
				      (const void **) &pam_user);
			if (pam_user[0] == '\0')
				pam_set_item (pamh, PAM_USER, NULL);

			/*
			 * There may be better ways to deal with some of
			 * these conditions, but at least this way I don't
			 * think we'll be giving away information. Perhaps
			 * someday we can trust that all PAM modules will
			 * pay attention to failure count and get rid of
			 * MAX_LOGIN_TRIES?
			 */
			failcount = 0;
			while (1) {
			  const char *failent_user;
			  failed = 0;

			  failcount++;
			  if (delay > 0)
			    retcode = pam_fail_delay(pamh, 1000000*delay);

			  retcode = pam_authenticate (pamh, 0);

			  pam_get_item (pamh, PAM_USER,
					(const void **) &pam_user);

			  if (pam_user && pam_user[0]) {
			    pwd = getpwnam(pam_user);
			    if (pwd) {
			      pwent = *pwd;
			      failent_user = pwent.pw_name;
			    } else {
			      if (getdef_bool("LOG_UNKFAIL_ENAB") && pam_user)
				failent_user = pam_user;
			      else
				failent_user = "UNKNOWN";
			    }
			  } else {
			    pwd = NULL;
			    failent_user = "UNKNOWN";
			  }

			  if (retcode == PAM_MAXTRIES || failcount >= retries) {
			    SYSLOG ((LOG_NOTICE,
				    "TOO MANY LOGIN TRIES (%d)%s FOR `%s'",
				    failcount, fromhost, failent_user));
			    fprintf(stderr,
				    _("Maximum number of tries exceeded (%d)\n"),
				    failcount);
			    PAM_END;
			    exit(0);
			  } else if (retcode == PAM_ABORT) {
			    /* Serious problems, quit now */
			    fprintf(stderr,_("login: abort requested by PAM\n"));
			    SYSLOG ((LOG_ERR,"PAM_ABORT returned from pam_authenticate()"));
			    PAM_END;
			    exit(99);
			  } else if (retcode != PAM_SUCCESS) {
			    SYSLOG ((LOG_NOTICE,"FAILED LOGIN (%d)%s FOR `%s', %s",
				   failcount, fromhost, failent_user,
				   pam_strerror (pamh, retcode)));
			    failed = 1;
			  }

			  if (!failed)
			    break;

#ifdef WITH_AUDIT
				{
					struct passwd *pw;
					char buf[64];

					audit_fd = audit_open ();
					pw = getpwnam (username);
					if (pw) {
						snprintf (buf, sizeof (buf),
						  "uid=%d", pw->pw_uid);
						audit_log_user_message
						    (audit_fd, AUDIT_USER_LOGIN,
						     buf, hostname, NULL,
						     tty, 0);
					} else {
						snprintf (buf, sizeof (buf),
							  "acct=%s", username);
						audit_log_user_message
						    (audit_fd, AUDIT_USER_LOGIN,
						     buf, hostname, NULL,
						     tty, 0);
					}
					close (audit_fd);
				}
#endif				/* WITH_AUDIT */

			  fprintf(stderr,"\nLogin incorrect\n");

			  /* Let's give it another go around */
			  pam_set_item(pamh,PAM_USER,NULL);
			}

			/* We don't get here unless they were authenticated above */
			retcode = pam_acct_mgmt (pamh, 0);

			if (retcode == PAM_NEW_AUTHTOK_REQD) {
				retcode =
				    pam_chauthtok (pamh,
						   PAM_CHANGE_EXPIRED_AUTHTOK);
			}

			PAM_FAIL_CHECK;
		}

		/* Grab the user information out of the password file for future usage
		   First get the username that we are actually using, though.
		 */
		retcode =
		    pam_get_item (pamh, PAM_USER, (const void **) &pam_user);
		setpwent ();
		pwd = getpwnam (pam_user);

		if (fflg) {
			retcode = pam_acct_mgmt (pamh, 0);
			PAM_FAIL_CHECK;
		}

		if (!pwd || setup_groups (pwd))
			exit (1);
		else
			pwent = *pwd;

		retcode = pam_setcred (pamh, PAM_ESTABLISH_CRED);
		PAM_FAIL_CHECK;

		retcode = pam_open_session (pamh,
					    hushed (&pwent) ? PAM_SILENT : 0);
		PAM_FAIL_CHECK;

#else				/* ! USE_PAM */
		while (1) {	/* repeatedly get login/password pairs */
			failed = 0;	/* haven't failed authentication yet */
			if (!username[0]) {	/* need to get a login id */
				if (subroot) {
					closelog ();
					exit (1);
				}
				preauth_flag = 0;
				login_prompt (_("\n%s login: "), username,
					      sizeof username);
				continue;
			}
#endif				/* ! USE_PAM */

#ifdef USE_PAM
		if (!(pwd = getpwnam (pam_user))) {
			pwent.pw_name = pam_user;
#else
		if (!(pwd = getpwnam (username))) {
			pwent.pw_name = username;
#endif
			strcpy (temp_pw, "!");
			pwent.pw_passwd = temp_pw;
			pwent.pw_shell = temp_shell;

			preauth_flag = 0;
			failed = 1;
		} else {
			pwent = *pwd;
		}
#ifndef USE_PAM
		spwd = NULL;
		if (pwd && strcmp (pwd->pw_passwd, SHADOW_PASSWD_STRING) == 0) {
			spwd = getspnam (username);
			if (spwd)
				pwent.pw_passwd = spwd->sp_pwdp;
			else
				SYSLOG ((LOG_WARN,
					 "no shadow password for `%s'%s",
					 username, fromhost));
		}

		/*
		 * If the encrypted password begins with a "!", the account
		 * is locked and the user cannot login, even if they have
		 * been "pre-authenticated."
		 */
		if (pwent.pw_passwd[0] == '!' || pwent.pw_passwd[0] == '*')
			failed = 1;

		/*
		 * The -r and -f flags provide a name which has already
		 * been authenticated by some server.
		 */
		if (preauth_flag)
			goto auth_ok;

		if (pw_auth
		    (pwent.pw_passwd, username, reason, (char *) 0) == 0)
			goto auth_ok;

		/*
		 * Don't log unknown usernames - I mistyped the password for
		 * username at least once. Should probably use LOG_AUTHPRIV
		 * for those who really want to log them.  --marekm
		 */
		SYSLOG ((LOG_WARN, "invalid password for `%s' %s",
			 (pwd
			  || getdef_bool ("LOG_UNKFAIL_ENAB")) ?
			 username : "UNKNOWN", fromhost));
		failed = 1;

	      auth_ok:
		/*
		 * This is the point where all authenticated users wind up.
		 * If you reach this far, your password has been
		 * authenticated and so on.
		 */
		if (!failed && pwent.pw_name && pwent.pw_uid == 0
		    && !is_console) {
			SYSLOG ((LOG_CRIT, "ILLEGAL ROOT LOGIN %s", fromhost));
			failed = 1;
		}
		if (!failed
		    && !login_access (username, *hostname ? hostname : tty)) {
			SYSLOG ((LOG_WARN, "LOGIN `%s' REFUSED %s",
				 username, fromhost));
			failed = 1;
		}
		if (pwd && getdef_bool ("FAILLOG_ENAB") &&
		    !failcheck (pwent.pw_uid, &faillog, failed)) {
			SYSLOG ((LOG_CRIT,
				 "exceeded failure limit for `%s' %s",
				 username, fromhost));
			failed = 1;
		}
		if (!failed)
			break;

		/* don't log non-existent users */
		if (pwd && getdef_bool ("FAILLOG_ENAB"))
			failure (pwent.pw_uid, tty, &faillog);
		if (getdef_str ("FTMP_FILE") != NULL) {
			const char *failent_user;

#if HAVE_UTMPX_H
			failent = utxent;
			if (sizeof (failent.ut_tv) == sizeof (struct timeval))
				gettimeofday ((struct timeval *)
					      &failent.ut_tv, NULL);
			else {
				struct timeval tv;

				gettimeofday (&tv, NULL);
				failent.ut_tv.tv_sec = tv.tv_sec;
				failent.ut_tv.tv_usec = tv.tv_usec;
			}
#else
			failent = utent;
			failent.ut_time = time (NULL);
#endif
			if (pwd) {
				failent_user = pwent.pw_name;
			} else {
				if (getdef_bool ("LOG_UNKFAIL_ENAB"))
					failent_user = username;
				else
					failent_user = "UNKNOWN";
			}
			strncpy (failent.ut_user, failent_user,
				 sizeof (failent.ut_user));
#ifdef USER_PROCESS
			failent.ut_type = USER_PROCESS;
#endif
			failtmp (&failent);
		}
		memzero (username, sizeof username);

		if (--retries <= 0)
			SYSLOG ((LOG_CRIT, "REPEATED login failures%s",
				 fromhost));
		/*
		 * If this was a passwordless account and we get here, login
		 * was denied (securetty, faillog, etc.). There was no
		 * password prompt, so do it now (will always fail - the bad
		 * guys won't see that the passwordless account exists at
		 * all).  --marekm
		 */
		if (pwent.pw_passwd[0] == '\0')
			pw_auth ("!", username, reason, (char *) 0);

		/*
		 * Wait a while (a la SVR4 /usr/bin/login) before attempting
		 * to login the user again. If the earlier alarm occurs
		 * before the sleep() below completes, login will exit.
		 */
		if (delay > 0)
			sleep (delay);

		puts (_("Login incorrect"));

		/* allow only one attempt with -r or -f */
		if (rflg || fflg || retries <= 0) {
			closelog ();
			exit (1);
		}
	}			/* while (1) */
#endif				/* ! USE_PAM */
	alarm (0);		/* turn off alarm clock */
#ifndef USE_PAM			/* PAM does this */
	/*
	 * porttime checks moved here, after the user has been
	 * authenticated. now prints a message, as suggested
	 * by Ivan Nejgebauer <ian@unsux.ns.ac.yu>.  --marekm
	 */
	if (getdef_bool ("PORTTIME_CHECKS_ENAB") &&
	    !isttytime (pwent.pw_name, tty, time ((time_t *) 0))) {
		SYSLOG ((LOG_WARN, "invalid login time for `%s'%s",
			 username, fromhost));
		closelog ();
		bad_time_notify ();
		exit (1);
	}

	check_nologin ();
#endif

	if (getenv ("IFS"))	/* don't export user IFS ... */
		addenv ("IFS= \t\n", NULL);	/* ... instead, set a safe IFS */

#ifdef USE_PAM
	setutmp (pam_user, tty, hostname);	/* make entry in utmp & wtmp files */
#else
	setutmp (username, tty, hostname);	/* make entry in utmp & wtmp files */
#endif
	if (pwent.pw_shell[0] == '*') {	/* subsystem root */
		pwent.pw_shell++;	/* skip the '*' */
		subsystem (&pwent);	/* figure out what to execute */
		subroot++;	/* say i was here again */
		endpwent ();	/* close all of the file which were */
		endgrent ();	/* open in the original rooted file */
		endspent ();	/* system. they will be re-opened */
#ifdef	SHADOWGRP
		endsgent ();	/* in the new rooted file system */
#endif
		goto top;	/* go do all this all over again */
	}

#ifdef WITH_AUDIT
	{
		char buf[32];

		audit_fd = audit_open ();
		snprintf (buf, sizeof (buf), "uid=%d", pwd->pw_uid);
		audit_log_user_message (audit_fd, AUDIT_USER_LOGIN,
					buf, hostname, NULL, tty, 1);
		close (audit_fd);
	}
#endif				/* WITH_AUDIT */

#ifndef USE_PAM			/* pam_lastlog handles this */
	if (getdef_bool ("LASTLOG_ENAB"))	/* give last login and log this one */
		dolastlog (&lastlog, &pwent, utent.ut_line, hostname);
#endif

#ifndef USE_PAM			/* PAM handles this as well */
	/*
	 * Have to do this while we still have root privileges, otherwise we
	 * don't have access to /etc/shadow. expire() closes password files,
	 * and changes to the user in the child before executing the passwd
	 * program.  --marekm
	 */
	if (spwd) {		/* check for age of password */
		if (expire (&pwent, spwd)) {
			pwd = getpwnam (username);
			spwd = getspnam (username);
			if (pwd)
				pwent = *pwd;
		}
	}
	setup_limits (&pwent);	/* nice, ulimit etc. */
#endif				/* ! USE_PAM */
	chown_tty (tty, &pwent);

#ifdef USE_PAM
	/*
	 * We must fork before setuid() because we need to call
	 * pam_close_session() as root.
	 */
	signal (SIGINT, SIG_IGN);
	child = fork ();
	if (child < 0) {
		/* error in fork() */
		fprintf (stderr, _("%s: failure forking: %s"),
			 Prog, strerror (errno));
		PAM_END;
		exit (0);
	} else if (child) {
		/*
		 * parent - wait for child to finish, then cleanup
		 * session
		 */
		wait (NULL);
		PAM_END;
		exit (0);
	}
	/* child */
#endif

	/* We call set_groups() above because this clobbers pam_groups.so */
#ifndef USE_PAM
	if (setup_uid_gid (&pwent, is_console))
#else
	if (change_uid (&pwent))
#endif
		exit (1);

	setup_env (&pwent);	/* set env vars, cd to the home dir */

#ifdef USE_PAM
	{
		const char *const *env;

		env = (const char *const *) pam_getenvlist (pamh);
		while (env && *env) {
			addenv (*env, NULL);
			env++;
		}
	}
#endif

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (!hushed (&pwent)) {
		addenv ("HUSHLOGIN=FALSE", NULL);
		/*
		 * pam_unix, pam_mail and pam_lastlog should take care of
		 * this
		 */
#ifndef USE_PAM
		motd ();	/* print the message of the day */
		if (getdef_bool ("FAILLOG_ENAB")
		    && faillog.fail_cnt != 0) {
			failprint (&faillog);
			/* Reset the lockout times if logged in */
			if (faillog.fail_max &&
			    faillog.fail_cnt >= faillog.fail_max) {
				puts (_
				      ("Warning: login re-enabled after temporary lockout.\n"));
				SYSLOG ((LOG_WARN,
					 "login `%s' re-enabled after temporary lockout (%d failures)",
					 username, (int) faillog.fail_cnt));
			}
		}
		if (getdef_bool ("LASTLOG_ENAB")
		    && lastlog.ll_time != 0) {
			time_t ll_time = lastlog.ll_time;

#ifdef HAVE_STRFTIME
			strftime (ptime, sizeof (ptime),
				  "%a %b %e %H:%M:%S %z %Y",
				  localtime (&ll_time));
			printf (_("Last login: %s on %s"),
				ptime, lastlog.ll_line);
#else
			printf (_("Last login: %.19s on %s"),
				ctime (&ll_time), lastlog.ll_line);
#endif
#ifdef HAVE_LL_HOST		/* __linux__ || SUN4 */
			if (lastlog.ll_host[0])
				printf (_(" from %.*s"),
					(int) sizeof lastlog.
					ll_host, lastlog.ll_host);
#endif
			printf (".\n");
		}
		agecheck (&pwent, spwd);

		mailcheck ();	/* report on the status of mail */
#endif				/* !USE_PAM */
	} else
		addenv ("HUSHLOGIN=TRUE", NULL);

	if (getdef_str ("TTYTYPE_FILE") != NULL && getenv ("TERM") == NULL)
		ttytype (tty);

	signal (SIGQUIT, SIG_DFL);	/* default quit signal */
	signal (SIGTERM, SIG_DFL);	/* default terminate signal */
	signal (SIGALRM, SIG_DFL);	/* default alarm signal */
	signal (SIGHUP, SIG_DFL);	/* added this.  --marekm */
	signal (SIGINT, SIG_DFL);	/* default interrupt signal */

	endpwent ();		/* stop access to password file */
	endgrent ();		/* stop access to group file */
	endspent ();		/* stop access to shadow passwd file */
#ifdef	SHADOWGRP
	endsgent ();		/* stop access to shadow group file */
#endif
	if (pwent.pw_uid == 0)
		SYSLOG ((LOG_NOTICE, "ROOT LOGIN %s", fromhost));
	else if (getdef_bool ("LOG_OK_LOGINS"))
#ifdef USE_PAM
		SYSLOG ((LOG_INFO, "`%s' logged in %s", pam_user, fromhost));
#else
		SYSLOG ((LOG_INFO, "`%s' logged in %s", username, fromhost));
#endif
	closelog ();
	if ((tmp = getdef_str ("FAKE_SHELL")) != NULL)
		err = shell (tmp, pwent.pw_shell, newenvp); /* fake shell */
	else
		/* exec the shell finally */
		err = shell (pwent.pw_shell, (char *) 0, newenvp);
	exit (err == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	/* NOT REACHED */
	return 0;
}
