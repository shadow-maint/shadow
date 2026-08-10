/*
 * Copyright 1989, 1990, 1991, 1992, John F. Haugh II
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

#ifdef SVR4
#include <utmpx.h>
#else
#include <sys/types.h>
#include <utmp.h>
#endif
#include <signal.h>
#include <stdio.h>
#include "pwd.h"
#include <fcntl.h>
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#include <memory.h>
#endif
#include "config.h"
#include "pwauth.h"

#if defined(BSD) || defined(SUN)
#include <sgtty.h>
#define	USE_SGTTY	1
#endif
#if defined(USG) || defined(SUN4)
#ifdef	_POSIX_SOURCE
#include <termios.h>
#define	USE_TERMIOS	1
#else
#include <termio.h>
#define	USE_TERMIO	1
#endif
#endif

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)sulogin.c	3.12	13:04:09	27 Jul 1992";
#endif

char	name[BUFSIZ];
char	pass[BUFSIZ];
char	home[BUFSIZ];
char	prog[BUFSIZ];
char	mail[BUFSIZ];

struct	passwd	pwent;
#ifdef SVR4
struct	utmpx	utent;
#else
struct	utmp	utent;
#endif

#ifdef	USE_SGTTY
struct	sgttyb	termio;
#endif
#ifdef	USE_TERMIO
struct	termio	termio;
#endif
#ifdef	USE_TERMIOS
struct	termios	termio;
#endif

#ifndef	MAXENV
#define	MAXENV	64
#endif

char	*newenvp[MAXENV];
int	newenvc = 0;
int	maxenv = MAXENV;
extern	char	**environ;
extern	char	*getpass();

extern	char	*getdef_str();

#ifndef	ALARM
#define	ALARM	60
#endif

#ifndef	RETRIES
#define	RETRIES	3
#endif

catch (sig)
int	sig;
{
	exit (1);
}

/*ARGSUSED*/
int
main (argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
	char	*getenv ();
	char	*ttyname ();
	char	*getpass ();
	char	*tz ();
	char	*cp;

#ifdef	USE_SGTTY
	ioctl (0, TIOCGETP, &termio);
	termio.sg_flags |= (ECHO|CRMOD);
	termio.sg_flags &= ~(RAW|CBREAK);
	ioctl (0, TIOCSETN, &termio);
#endif
#ifdef	USE_TERMIO
	ioctl (0, TCGETA, &termio);
	termio.c_iflag |= (ICRNL|IXON);
	termio.c_oflag |= (OPOST|ONLCR);
	termio.c_cflag |= (CREAD);
	termio.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK);
	ioctl (0, TCSETAF, &termio);
#endif
#ifdef	USE_TERMIOS
	tcgetattr (0, &termio);
	termio.c_iflag |= (ICRNL|IXON);
	termio.c_oflag |= (CREAD);
	termio.c_lflag |= (ECHO|ECHOE|ECHOK|ICANON|ISIG);
	tcsetattr (0, TCSANOW, &termio);
#endif
#ifdef	USE_SYSLOG
	openlog ("sulogin", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif
	if (argc > 1) {
		close (0);
		close (1);
		close (2);

		if (open (argv[1], O_RDWR) >= 0) {
			dup (0);
			dup (0);
		} else {
#ifdef	USE_SYSLOG
			syslog (LOG_WARN, "cannot open %s\n", argv[1]);
			closelog ();
#endif
			exit (1);
		}
	}
	if (access (PWDFILE, 0) == -1) { /* must be a password file! */
		printf ("No password file\n");
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "No password file\n");
		closelog ();
#endif
		exit (1);
	}
#ifndef	DEBUG
	if (getppid () != 1) {		/* parent must be INIT */
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "Pid == %d, not 1\n", getppid ());
		closelog ();
#endif
		exit (1);
	}
#endif
	if (! isatty (0) || ! isatty (1) || ! isatty (2)) {
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);		/* must be a terminal */
	}
	while (*envp)			/* add inherited environment, */
		addenv (*envp++);	/* some variables change later */

	if (cp = getdef_str("ENV_TZ"))
		addenv (*cp == '/' ? tz(cp) : cp);
	if (cp = getdef_str("ENV_HZ"))
		addenv (cp);		/* set the default $HZ, if one */
	(void) strcpy (name, "root");	/* KLUDGE!!! */

	signal (SIGALRM, catch);	/* exit if the timer expires */
	alarm (ALARM);			/* only wait so long ... */

	while (1) {		/* repeatedly get login/password pairs */
		entry (name, &pwent);	/* get entry from password file */
		if (pwent.pw_name == (char *) 0) {

			/*
			 * Fail secure
			 */

			printf ("No password entry for 'root'\n");
#ifdef	USE_SYSLOG
			syslog (LOG_WARN, "No password entry for 'root'\n");
			closelog ();
#endif
			exit (1);
		}

	/*
	 * Here we prompt for the root password, or if no password is
	 * given we just exit.
	 */

					/* get a password for root */
		if (! (cp = getpass ("Type control-d for normal startup,\n\
(or give root password for system maintenance):"))) {
#ifdef	USE_SYSLOG
			syslog (LOG_INFO, "Normal startup\n");
			closelog ();
#endif
#ifdef	TELINIT
			execl ("/etc/telinit", "telinit", RUNLEVEL, (char *) 0);
#endif
			exit (0);
		} else
			strcpy (pass, cp);

		if (pwent.pw_name && pwent.pw_passwd[0] == '@') {
			if (pw_auth (pwent.pw_passwd + 1, name, PW_LOGIN)) {
#ifdef	USE_SYSLOG
				syslog (LOG_WARN,
					"Incorrect root authentication");
#endif
				continue;
			}
			goto auth_done;
		}
		if (valid (pass, &pwent)) /* check encrypted passwords ... */
			break;		/* ... encrypted passwords matched */

		puts ("Login incorrect");
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "Incorrect root password\n");
#endif
	}
auth_done:
	alarm (0);
	signal (SIGALRM, SIG_DFL);
	environ = newenvp;		/* make new environment active */

	puts ("Entering System Maintenance Mode");
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, "System Maintenance Mode\n");
#endif

	/*
	 * Normally there would be a utmp entry for login to mung on
	 * to get the tty name, date, etc. from.  We don't need all that
	 * stuff because we won't update the utmp or wtmp files.  BUT!,
	 * we do need the tty name so we can set the permissions and
	 * ownership.
	 */

	if (cp = ttyname (0)) {		/* found entry in /dev/ */
		if (strrchr (cp, '/') != (char *) 0)
			strcpy (utent.ut_line, strrchr (cp, '/') + 1);
		else
			strcpy (utent.ut_line, cp);
	}
	if (getenv ("IFS"))		/* don't export user IFS ... */
		addenv ("IFS= \t\n");	/* ... instead, set a safe IFS */

	setup (&pwent);			/* set UID, GID, HOME, etc ... */

#ifdef	USE_SYSLOG
	closelog ();
#endif
	shell (pwent.pw_shell, (char *) 0); /* exec the shell finally. */
	/*NOTREACHED*/
}
