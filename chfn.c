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
static	char	sccsid[] = "@(#)chfn.c	3.10	07:44:44	20 Apr 1993";
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

char	*Prog;
char	user[BUFSIZ];
char	fullnm[BUFSIZ];
char	roomno[BUFSIZ];
char	workph[BUFSIZ];
char	homeph[BUFSIZ];
char	slop[BUFSIZ];
int	amroot;

/*
 * External identifiers
 */

extern	int	optind;
extern	char	*optarg;
extern	struct	passwd	*getpwuid ();
extern	struct	passwd	*getpwnam ();
extern	char	*getlogin ();
#ifdef	NDBM
extern	int	pw_dbm_mode;
#endif

/*
 * #defines for messages.  This facilities foreign language conversion
 * since all messages are defined right here.
 */

#define	USAGE \
"Usage: %s [ -f full_name ] [ -r room_no ] [ -w work_ph ] [ -h home_ph ]\n"
#define	ADMUSAGE \
"Usage: %s [ -f full_name ] [ -r room_no ] [ -w work_ph ]\n\
       [ -h home_ph ] [ -o other ] [ user ]\n"
#define	NOPERM		"%s: Permission denied.\n"
#define	WHOAREYOU	"%s: Cannot determine you user name.\n"
#define	INVALID_NAME	"%s: invalid name: \"%s\"\n"
#define	INVALID_ROOM	"%s: invalid room number: \"%s\"\n"
#define	INVALID_WORKPH	"%s: invalid work phone: \"%s\"\n"
#define	INVALID_HOMEPH	"%s: invalid home phone: \"%s\"\n"
#define	INVALID_OTHER	"%s: \"%s\" contains illegal characters\n"
#define	INVALID_FIELDS	"%s: fields too long\n"
#define	NEWFIELDSMSG	"Changing the user information for %s\n"
#define	NEWFIELDSMSG2 \
"Enter the new value, or press return for the default\n\n"
#define	NEWNAME		"Full Name"
#define	NEWROOM		"Room Number"
#define	NEWWORKPHONE	"Work Phone"
#define	NEWHOMEPHONE	"Home Phone"
#define	NEWSLOP		"Other"
#define	UNKUSER		"%s: Unknown user %s\n"
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
#define	CHGGECOS	"changed user `%s' information.\n"

/*
 * usage - print command line syntax and exit
 */

void
usage ()
{
	fprintf (stderr, amroot ? USAGE:ADMUSAGE, Prog);
	exit (1);
}

/*
 * new_fields - change the user's GECOS information interactively
 *
 * prompt the user for each of the four fields and fill in the fields
 * from the user's response, or leave alone if nothing was entered.
 */

new_fields ()
{
	printf (NEWFIELDSMSG2);

	change_field (fullnm, NEWNAME);
	change_field (roomno, NEWROOM);
	change_field (workph, NEWWORKPHONE);
	change_field (homeph, NEWHOMEPHONE);

	if (amroot)
		change_field (slop, NEWSLOP);
}

/*
 * copy_field - get the next field from the gecos field
 *
 * copy_field copies the next field from the gecos field, returning a
 * pointer to the field which follows, or NULL if there are no more
 * fields.
 */

char *
copy_field (in, out, extra)
char	*in;			/* the current GECOS field */
char	*out;			/* where to copy the field to */
char	*extra;			/* fields with '=' get copied here */
{
	char	*cp;

	while (in) {
		if (cp = strchr (in, ','))
			*cp++ = '\0';

		if (! strchr (in, '='))
			break;

		if (extra) {
			if (extra[0])
				strcat (extra, ",");

			strcat (extra, in);
		}
		in = cp;
	}
	if (in && out)
		strcpy (out, in);

	return cp;
}

/*
 * chfn - change a user's password file information
 *
 *	This command controls the GECOS field information in the
 *	password file entry.
 *
 *	The valid options are
 *
 *	-f	full name
 *	-r	room number
 *	-w	work phone number
 *	-h	home phone number
 *	-o	other information (*)
 *
 *	(*) requires root permission to execute.
 */

int
main (argc, argv)
int	argc;
char	**argv;
{
	char	*cp;			/* temporary character pointer       */
	struct	passwd	*pw;		/* password file entry               */
	struct	passwd	pwent;		/* modified password file entry      */
	char	old_gecos[BUFSIZ];	/* buffer for old GECOS fields       */
	char	new_gecos[BUFSIZ];	/* buffer for new GECOS fields       */
	int	flag;			/* flag currently being processed    */
	int	fflg = 0;		/* -f - set full name                */
	int	rflg = 0;		/* -r - set room number              */
	int	wflg = 0;		/* -w - set work phone number        */
	int	hflg = 0;		/* -h - set home phone number        */
	int	oflg = 0;		/* -o - set other information        */
	int	i;			/* loop control variable             */

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
	 * The remaining arguments will be processed one by one and
	 * executed by this command.  The name is the last argument
	 * if it does not begin with a "-", otherwise the name is
	 * determined from the environment and must agree with the
	 * real UID.  Also, the UID will be checked for any commands
	 * which are restricted to root only.
	 */

	while ((flag = getopt (argc, argv, "f:r:w:h:o:")) != EOF) {
		switch (flag) {
			case 'f':
				fflg++;
				strcpy (fullnm, optarg);
				break;
			case 'r':
				rflg++;
				strcpy (roomno, optarg);
				break;
			case 'w':
				wflg++;
				strcpy (workph, optarg);
				break;
			case 'h':
				hflg++;
				strcpy (homeph, optarg);
				break;
			case 'o':
				if (amroot) {
					oflg++;
					strcpy (slop, optarg);
					break;
				}
				fprintf (stderr, NOPERM, Prog);
#ifdef	USE_SYSLOG
				closelog ();
#endif
				exit (1);
			default:
				usage ();
		}
	}

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
		fprintf (stderr, NOPERM, Prog);
#ifdef	USE_SYSLOG
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
	pwent.pw_shell = strdup (pw->pw_shell);

	/*
	 * Now get the full name.  It is the first comma separated field
	 * in the GECOS field.
	 */

	strcpy (old_gecos, pw->pw_gecos);
	cp = copy_field (old_gecos, fflg ? (char *) 0:fullnm, slop);

	/*
	 * Now get the room number.  It is the next comma separated field,
	 * if there is indeed one.
	 */

	if (cp)
		cp = copy_field (cp, rflg ? (char *) 0:roomno, slop);

	/*
	 * Now get the work phone number.  It is the third field.
	 */

	if (cp)
		cp = copy_field (cp, wflg ? (char *) 0:workph, slop);

	/*
	 * Now get the home phone number.  It is the fourth field.
	 */

	if (cp)
		cp = copy_field (cp, hflg ? (char *) 0:homeph, slop);

	/*
	 * Anything left over is "slop".
	 */

	if (cp) {
		if (slop[0])
			strcat (slop, ",");

		strcat (slop, cp);
	}

	/*
	 * If none of the fields were changed from the command line,
	 * let the user interactively change them.
	 */

	if (! fflg && ! rflg && ! wflg && ! hflg && ! oflg) {
		printf (NEWFIELDSMSG, user);
		new_fields ();
	}

	/*
	 * Check all of the fields for valid information
	 */

	if (valid_field (fullnm, ":,=")) {
		fprintf (stderr, INVALID_NAME, Prog, fullnm);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (valid_field (roomno, ":,=")) {
		fprintf (stderr, INVALID_ROOM, Prog, roomno);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (valid_field (workph, ":,=")) {
		fprintf (stderr, INVALID_WORKPH, Prog, workph);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (valid_field (homeph, ":,=")) {
		fprintf (stderr, INVALID_HOMEPH, Prog, homeph);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	if (valid_field (slop, ":")) {
		fprintf (stderr, INVALID_OTHER, Prog, slop);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}

	/*
	 * Build the new GECOS field by plastering all the pieces together,
	 * if they will fit ...
	 */

	if (strlen (fullnm) + strlen (roomno) + strlen (workph) +
			strlen (homeph) + strlen (slop) > (unsigned int) 80) {
		fprintf (stderr, INVALID_FIELDS, Prog);
#ifdef	USE_SYSLOG
		closelog ();
#endif
		exit (1);
	}
	sprintf (new_gecos, "%s,%s,%s,%s", fullnm, roomno, workph, homeph);
	if (slop[0]) {
		strcat (new_gecos, ",");
		strcat (new_gecos, slop);
	}
	pwent.pw_gecos = new_gecos;
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
	syslog (LOG_INFO, CHGGECOS, user);
	closelog ();
#endif
	exit (0);
}
