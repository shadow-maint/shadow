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

#ifndef	lint
static	char	sccsid[] = "@(#)chsh.c	3.9	07:46:32	20 Apr 1993";
#endif

/*
 * Set up some BSD defines so that all the BSD ifdef's are
 * kept right here 
 */

#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

#include "config.h"
#include "pwd.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif
#endif
#ifdef	HAVE_RLIMIT
#include <sys/resource.h>

struct	rlimit	rlimit_fsize = { RLIM_INFINITY, RLIM_INFINITY };
#endif

/*
 * Global variables.
 */

char	*Prog;			/* Program name */
int	amroot;				/* Real UID is root */
char	loginsh[BUFSIZ];		/* Name of new login shell */

/*
 * External identifiers
 */

extern	struct	passwd	*getpwuid ();
extern	struct	passwd	*getpwnam ();
extern	void	change_field ();
extern	int	optind;
extern	char	*optarg;
extern	char	*getlogin ();
#ifdef	NDBM
extern	int	pw_dbm_mode;
#endif

/*
 * #defines for messages.  This facilitates foreign language conversion
 * since all messages are defined right here.
 */

#define	USAGE		"Usage: %s [ -s shell ] [ name ]\n"
#define	WHOAREYOU	"%s: Cannot determine your user name.\n"
#define	UNKUSER		"%s: Unknown user %s\n"
#define	NOPERM		"You may not change the shell for %s.\n"
#define	NOPERM2		"can't change shell for `%s'\n"
#define	NEWSHELLMSG	"Changing the login shell for %s\n"
#define	NEWSHELL	"Login Shell"
#define	NEWSHELLMSG2 \
	"Enter the new value, or press return for the default\n\n"
#define	BADSHELL	"%s is an invalid shell.\n"
#define	BADFIELD	"%s: Invalid entry: %s\n"
#define	PWDBUSY		"Cannot lock the password file; try again later.\n"
#define	PWDBUSY2	"can't lock /etc/passwd\n"
#define	OPNERROR	"Cannot open the password file.\n"
#define	OPNERROR2	"can't open /etc/passwd\n"
#define	UPDERROR	"Error updating the password entry.\n"
#define	UPDERROR2	"error updating passwd entry\n"
#define	DBMERROR	"Error updating the DBM password entry.\n"
#define	DBMERROR2	"error updating DBM passwd entry.\n"
#define	NOTROOT		"Cannot change ID to root.\n"
#define	NOTROOT2	"can't setuid(0).\n"
#define	CLSERROR	"Cannot commit password file changes.\n"
#define	CLSERROR2	"can't rewrite /etc/passwd.\n"
#define	UNLKERROR	"Cannot unlock the password file.\n"
#define	UNLKERROR2	"can't unlock /etc/passwd.\n"
#define	CHGSHELL	"changed user `%s' shell to `%s'\n"

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
 * new_fields - change the user's login shell information interactively
 *
 * prompt the user for the login shell and change it according to the
 * response, or leave it alone if nothing was entered.
 */

new_fields ()
{
	printf (NEWSHELLMSG2);
	change_field (loginsh, NEWSHELL);
}

/*
 * check_shell - see if the user's login shell is listed in /etc/shells
 *
 * The /etc/shells file is read for valid names of login shells.  If the
 * /etc/shells file does not exist the user cannot set any shell unless
 * they are root.
 */

check_shell (shell)
char	*shell;
{
	char	buf[BUFSIZ];
	char	*cp;
	int	found = 0;
	FILE	*fp;

	if (amroot)
		return 1;

	if ((fp = fopen ("/etc/shells", "r")) == (FILE *) 0)
		return 0;

	while (fgets (buf, BUFSIZ, fp) && ! found) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (strcmp (buf, shell) == 0)
			found = 1;
	}
	fclose (fp);

	return found;
}

/*
 * restricted_shell - return true if the named shell begins with 'r' or 'R'
 *
 * If the first letter of the filename is 'r' or 'R', the shell is
 * considered to be restricted.
 */

int
restricted_shell (shell)
char	*shell;
{
	char	*cp;

	if (cp = strrchr (shell, '/'))
		cp++;
	else
		cp = shell;

	return *cp == 'r' || *cp == 'R';
}

/*
 * chsh - this command controls changes to the user's shell
 *
 *	The only supported option is -s which permits the
 *	the login shell to be set from the command line.
 */

int
main (argc, argv)
int	argc;
char	**argv;
{
	char	user[BUFSIZ];		/* User name                         */
	int	flag;			/* Current command line flag         */
	int	sflg = 0;		/* -s - set shell from command line  */
	int	i;			/* Loop control variable             */
	char	*cp;			/* Miscellaneous character pointer   */
	struct	passwd	*pw;		/* Password entry from /etc/passwd   */
	struct	passwd	pwent;		/* New password entry                */

	/*
	 * This command behaves different for root and non-root
	 * users.
	 */

	amroot = getuid () == 0;
#ifdef	NDBM
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
	openlog (Prog, LOG_PID, LOG_AUTH);
#endif

	/*
	 * There is only one option, but use getopt() anyway to
	 * keep things consistent.
	 */

	while ((flag = getopt (argc, argv, "s:")) != EOF) {
		switch (flag) {
			case 's':
				sflg++;
				strcpy (loginsh, optarg);
				break;
			default:
				usage ();
		}
	}

	/*
	 * There should be only one remaining argument at most
	 * and it should be the user's name.
	 */

	if (argc > optind + 1)
		usage ();

	/*
	 * Get the name of the user to check.  It is either
	 * the command line name, or the name getlogin()
	 * returns.
	 */

	if (optind < argc) {
		strncpy (user, argv[optind], sizeof user);
		pw = getpwnam (user);
	} else if (cp = getlogin ()) {
		strncpy (user, cp, sizeof user);
		pw = getpwnam (user);
	} else {
		fprintf (stderr, WHOAREYOU, Prog);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Make certain there was a password entry for the
	 * user.
	 */

	if (! pw) {
		fprintf (stderr, UNKUSER, Prog, user);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Non-privileged users are only allowed to change the
	 * shell if the UID of the user matches the current
	 * real UID.
	 */

	if (! amroot && pw->pw_uid != getuid ()) {
		fprintf (stderr, NOPERM, user);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, NOPERM2, user);
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Non-privileged users are only allowed to change the
	 * shell if it is not a restricted one.
	 */

	if (! amroot && restricted_shell (pw->pw_shell)) {
		fprintf (stderr, NOPERM, user);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, NOPERM2, user);
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Make a copy of the user's password file entry so it
	 * can be modified without worrying about it be modified
	 * elsewhere.
	 */

	pwent = *pw;
	pwent.pw_name = strdup (pw->pw_name);
	pwent.pw_passwd = strdup (pw->pw_passwd);
#ifdef	ATT_AGE
	pwent.pw_age = strdup (pw->pw_age);
#endif
#ifdef	ATT_COMMENT
	pwent.pw_comment = strdup (pw->pw_comment);
#endif
	pwent.pw_dir = strdup (pw->pw_dir);
	pwent.pw_gecos = strdup (pw->pw_gecos);

	/*
	 * Now get the login shell.  Either get it from the password
	 * file, or use the value from the command line.
	 */

	if (! sflg)
		strcpy (loginsh, pw->pw_shell);

	/*
	 * If the login shell was not set on the command line,
	 * let the user interactively change it.
	 */

	if (! sflg) {
		printf (NEWSHELLMSG, user);
		new_fields ();
	}

	/*
	 * Check all of the fields for valid information.  The shell
	 * field may not contain any illegal characters.  Non-privileged
	 * users are restricted to using the shells in /etc/shells.
	 */

	if (valid_field (loginsh, ":,=")) {
		fprintf (stderr, BADFIELD, Prog, loginsh);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (! check_shell (loginsh)) {
		fprintf (stderr, BADSHELL, loginsh);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	pwent.pw_shell = loginsh;
	pw = &pwent;

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals.  Any
	 * keyboard signals are set to be ignored.
	 */

#ifdef	HAVE_ULIMIT
	ulimit (2, 30000);
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
	 * The passwd entry is now ready to be committed back to
	 * the password file.  Get a lock on the file and open it.
	 */

	for (i = 0;i < 30;i++)
		if (pw_lock ())
			break;

	if (i == 30) {
		fprintf (stderr, PWDBUSY);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, PWDBUSY2);
		closelog ();
#endif
		exit (1);
	}
	if (! pw_open (O_RDWR)) {
		fprintf (stderr, OPNERROR);
		(void) pw_unlock ();
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, OPNERROR2);
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Update the passwd file entry.  If there is a DBM file,
	 * update that entry as well.
	 */

	if (! pw_update (pw)) {
		fprintf (stderr, UPDERROR);
		(void) pw_unlock ();
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, UPDERROR2);
		closelog ();
#endif
		exit (1);
	}
#if defined(DBM) || defined(NDBM)
	if (access ("/etc/passwd.pag", 0) == 0 && ! pw_dbm_update (pw)) {
		fprintf (stderr, DBMERROR);
		(void) pw_unlock ();
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, DBMERROR2);
		closelog ();
#endif
		exit (1);
	}
	endpwent ();
#endif

	/*
	 * Changes have all been made, so commit them and unlock the
	 * file.
	 */

	if (! pw_close ()) {
		fprintf (stderr, CLSERROR);
		(void) pw_unlock ();
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, CLSERROR2);
		closelog ();
#endif
		exit (1);
	}
	if (! pw_unlock ()) {
		fprintf (stderr, UNLKERROR);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, UNLKERROR2);
		closelog ();
#endif
		exit (1);
	}
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, CHGSHELL, user, pwent.pw_shell);
	closelog ();
#endif
	exit (0);
}
