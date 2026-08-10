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

#ifndef	lint
static	char	sccsid[] = "@(#)smain.c	3.14	07:57:10	06 May 1993";
#endif

/*
 * Set up some BSD defines so that all the BSD ifdef's are
 * kept right here 
 */

#include "config.h"
#if defined(USG) || defined(SUN4)
#include <string.h>
#include <memory.h>
#define	bzero(a,n)	memset(a, 0, n)
#include <termio.h>
#else
#include <strings.h>
#include <sgtty.h>
#define	strchr	index
#define	strrchr	rindex
#endif

#include <signal.h>
#include "lastlog.h"
#include "pwd.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#endif
#include "pwauth.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

/*VARARGS*/ int syslog();

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif	/* !LOG_WARN */
#endif	/* USE_SYSLOG */

/*
 * Password aging constants
 *
 *	DAY - seconds in a day
 *	WEEK - seconds in a week
 *	SCALE - convert from clock to aging units
 */

#define	DAY	(24L*3600L)
#define	WEEK	(7L*DAY)

#ifdef	ITI_AGING
#define	SCALE	(1)
#else
#define	SCALE	DAY
#endif

/*
 * Assorted #defines to control su's behavior
 */

#ifndef	MAXENV
#define	MAXENV	128
#endif

/*
 * Global variables
 */

char	hush[BUFSIZ];
char	name[BUFSIZ];
char	pass[BUFSIZ];
char	home[BUFSIZ];
char	prog[BUFSIZ];
char	mail[BUFSIZ];
char	oldname[BUFSIZ];
char	*newenvp[MAXENV];
char	*Prog;
int	newenvc = 0;
int	maxenv = MAXENV;
struct	passwd	pwent;

/*
 * External identifiers
 */

extern	void	addenv ();
extern	void	entry ();
extern	void	sulog ();
extern	void	subsystem ();
extern	void	setup ();
extern	void	motd ();
extern	void	mailcheck ();
extern	void	shell ();
extern	char	*ttyname ();
extern	char	*getenv ();
extern	char	*getpass ();
extern	char	*tz ();
extern	char	*pw_encrypt();
extern	int	pw_auth();
extern	struct	passwd	*getpwuid ();
extern	struct	passwd	*getpwnam ();
extern	struct	spwd	*getspnam ();
extern	char	*getdef_str();
extern	int	getdef_bool();
extern	char	**environ;

/*
 * die - set or reset termio modes.
 *
 *	die() is called before processing begins.  signal() is then
 *	called with die() as the signal handler.  If signal later
 *	calls die() with a signal number, the terminal modes are
 *	then reset.
 */

void	die (killed)
int	killed;
{
#if defined(BSD) || defined(SUN)
	static	struct	sgttyb	sgtty;

	if (killed)
		stty (0, &sgtty);
	else
		gtty (0, &sgtty);
#else
	static	struct	termio	sgtty;

	if (killed)
		ioctl (0, TCSETA, &sgtty);
	else
		ioctl (0, TCGETA, &sgtty);
#endif
	if (killed) {
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (killed);
	}
}

/*
 * su - switch user id
 *
 *	su changes the user's ids to the values for the specified user.
 *	if no new user name is specified, "root" is used by default.
 *
 *	The only valid option is a "-" character, which is interpreted
 *	as requiring a new login session to be simulated.
 *
 *	Any additional arguments are passed to the user's shell.  In
 *	particular, the argument "-c" will cause the next argument to
 *	be interpreted as a command by the common shell programs.
 */

int	main (argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
	SIGTYPE	(*oldsig)();
	char	*cp;
	char	arg0[64];
	char	*tty = 0;		/* Name of tty SU is run from        */
	int	doshell = 0;
	int	fakelogin = 0;
	int	amroot = 0;
	struct	passwd	*pw = 0;
#ifdef	SHADOWPWD
	struct	spwd	*spwd = 0;
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
	 * Get the tty name.  Entries will be logged indicating that
	 * the user tried to change to the named new user from the
	 * current terminal.
	 */

	if (isatty (0) && (cp = ttyname (0))) {
		if (strncmp (cp, "/dev/", 5) == 0)
			tty = cp + 5;
		else
			tty = cp;
	} else
		tty = "???";

	/*
	 * Process the command line arguments. 
	 */

	argc--; argv++;			/* shift out command name */

	if (argc > 0 && argv[0][0] == '-' && argv[0][1] == '\0') {
		fakelogin = 1;
		argc--; argv++;		/* shift ... */
	}

	/*
	 * If a new login is being set up, the old environment will
	 * be ignored and a new one created later on.
	 */

	if (! fakelogin)
		while (*envp)
			addenv (*envp++);

	if (fakelogin && (cp=getdef_str("ENV_TZ")))
		addenv (*cp == '/' ? tz(cp) : cp);

	/*
	 * The clock frequency will be reset to the login value if required
	 */

	if (fakelogin && (cp=getdef_str("ENV_HZ")) )
		addenv (cp);		/* set the default $HZ, if one */

	/*
	 * The terminal type will be left alone if it is present in the
	 * environment already.
	 */

	if (fakelogin && (cp = getenv ("TERM"))) {
		char	term[BUFSIZ];

		sprintf (term, "TERM=%s", cp);
		addenv (term);
	}

	/*
	 * The next argument must be either a user ID, or some flag to
	 * a subshell.  Pretty sticky since you can't have an argument
	 * which doesn't start with a "-" unless you specify the new user
	 * name.  Any remaining arguments will be passed to the user's
	 * login shell.
	 */

	if (argc > 0 && argv[0][0] != '-') {
		(void) strcpy (name, argv[0]);	/* use this login id */
		argc--; argv++;		/* shift ... */
	}
	if (! name[0]) 			/* use default user ID */
		(void) strcpy (name, "root");

	doshell = argc == 0;		/* any arguments remaining? */

	/*
	 * Get the user's real name.  The current UID is used to determine
	 * who has executed su.  That user ID must exist.
	 */

	if (pw = getpwuid (getuid ()))	/* need old user name */
		(void) strcpy (oldname, pw->pw_name);
	else {				/* user ID MUST exist */ 
#ifdef	USE_SYSLOG
		syslog (LOG_CRIT, "Unknown UID: %d\n", getuid ());
#endif
		goto failure;
	}
	amroot = getuid () == 0;	/* currently am super user */

top:
	/*
	 * This is the common point for validating a user whose name
	 * is known.  It will be reached either by normal processing,
	 * or if the user is to be logged into a subsystem root.
	 *
	 * The password file entries for the user is gotten and the
	 * account validated.
	 */

	if (pw = getpwnam (name)) {
		if (spwd = getspnam (name))
			pw->pw_passwd = spwd->sp_pwdp;
	} else {
		(void) fprintf (stderr, "Unknown id: %s\n", name);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	pwent = *pw;

	/*
	 * Set the default shell.
	 */

	if (pwent.pw_shell[0] == '\0')
		pwent.pw_shell = "/bin/sh";

	/*
	 * Set up a signal handler in case the user types QUIT.
	 */

	die (0);
	oldsig = signal (SIGQUIT, die);

	/*
	 * See if the system defined authentication method is being used.
	 * The first character of an administrator defined method is an
	 * '@' character.
	 */

	if (! amroot && pw_auth (pwent.pw_passwd, name, PW_SU, (char *) 0)) {
#ifdef	USE_SYSLOG
		syslog (pwent.pw_uid ? LOG_WARN:LOG_CRIT,
			"Authentication failed for %s\n", name);
#endif
failure:
		sulog (tty, 0);		/* log failed attempt */
		puts ("Sorry.");
#ifdef	USE_SYSLOG
		if ( getdef_bool("SYSLOG_SU_ENAB") )
			syslog (pwent.pw_uid ? LOG_INFO:LOG_CRIT,
				"- %s %s-%s\n", tty,
				oldname[0] ? oldname:"???",
				name[0] ? name:"???");
		closelog ();
#endif
		exit (1);
	}
	(void) signal (SIGQUIT, oldsig);

	/*
	 * Check to see if the account is expired.  root gets to
	 * ignore any expired accounts, but normal users can't become
	 * a user with an expired password.
	 */

	if (! amroot) {
		if (spwd) {
			if (isexpired (&pwent, spwd)) {
#ifdef	USE_SYSLOG
				syslog (pwent.pw_uid ? LOG_WARN:LOG_CRIT,
					"Expired account %s\n", name);
#endif
				goto failure;
			}
		}
#ifdef	ATT_AGE
		else if (pwent.pw_age[0] &&
				isexpired (&pwent, (struct spwd *) 0)) {
#ifdef	USE_SYSLOG
			syslog (pwent.pw_uid ? LOG_WARN:LOG_CRIT,
				"Expired account %s\n", name);
#endif
			goto failure;
		}
#endif	/* ATT_AGE */
	}

	cp = getdef_str( pwent.pw_uid == 0 ? "ENV_SUPATH" : "ENV_PATH" );
	addenv( cp != NULL ? cp : "PATH=/bin:/usr/bin" );

	environ = newenvp;		/* make new environment active */

	if (getenv ("IFS"))		/* don't export user IFS ... */
		addenv ("IFS= \t\n");	/* ... instead, set a safe IFS */

	if (doshell && pwent.pw_shell[0] == '*') { /* subsystem root required */
		subsystem (&pwent);	/* figure out what to execute */
		endpwent ();
		endspent ();
		goto top;
	}

	sulog (tty, 1);			/* save SU information */
	endpwent ();
	endspent ();
#ifdef	USE_SYSLOG
	if ( getdef_bool("SYSLOG_SU_ENAB") )
		syslog (LOG_INFO, "+ %s %s-%s\n", tty,
			oldname[0] ? oldname:"???", name[0] ? name:"???");
#endif
	if (fakelogin)
		setup (&pwent);		/* set UID, GID, HOME, etc ... */
	else {
		if (setgid (pwent.pw_gid) || setuid (pwent.pw_uid))  {
			perror ("Can't set ID");
#ifdef	USE_SYSLOG
			syslog (LOG_CRIT, "Unable to set uid = %d, gid = %d\n",
				pwent.pw_uid, pwent.pw_gid);
			closelog ();
#endif
			exit (1);
		}
	}
	if (! doshell) {		/* execute arguments as command */
		if (cp = getenv ("SHELL"))
			pwent.pw_shell = cp;
		argv[-1] = pwent.pw_shell;
		(void) execv (pwent.pw_shell, &argv[-1]);
		(void) fprintf (stderr, "No shell\n");
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "Cannot execute %s\n", pwent.pw_shell);
		closelog ();
#endif
		exit (1);
	}
	if (fakelogin) {
		if (! hushed (&pwent)) {
			motd ();
			mailcheck ();
		}
		if ((cp = getdef_str("SU_NAME")) == NULL) {
			if (cp = strrchr (pwent.pw_shell, '/'))
				cp++;
			else
	    			cp = pwent.pw_shell;
		}
		arg0[0] = '-';
		strncpy(arg0+1, cp, sizeof(arg0)-1);
		arg0[sizeof(arg0)-1] = '\0';
		cp = arg0;
	} else {
		if (cp = strrchr (pwent.pw_shell, '/'))
			cp++;
		else
	    		cp = pwent.pw_shell;
	}

	shell (pwent.pw_shell, cp);
#ifdef	USE_SYSLOG
	syslog (LOG_WARN, "Cannot execute %s\n", pwent.pw_shell);
	closelog ();
#endif
	exit (1);

	/*NOTREACHED*/
}
