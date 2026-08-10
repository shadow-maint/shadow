/*
 * Copyright 1990, 1991, 1992, 1993, John F. Haugh II
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
#include "pwd.h"
#include "shadow.h"
#include <grp.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#if defined(USG) || defined(SUN4)
#include <termio.h>
#ifdef SYS3
# include <sys/ioctl.h>
#endif	/* SYS3 */
#include <string.h>
#ifndef	SYS3
# include <memory.h>
#endif /* !SYS3 */
#else /* SUN || BSD */
#include <sgtty.h>
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif /* !SUN && !BSD */
#include "config.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif

#ifdef	USG
#define	bzero(p,l) memset(p, 0, l)
#endif

#ifndef	lint
static	char	_sccsid[] = "@(#)gpmain.c	3.14	08:16:38	07 May 1993";
#endif

char	name[BUFSIZ];
char	pass[BUFSIZ];
char	pass2[BUFSIZ];

struct	group	grent;

char	*Prog;
char	*user;
char	*group;
char	*admins;
char	*members;

int	aflg;
int	Aflg;
int	dflg;
int	Mflg;
int	rflg;
int	Rflg;

#ifndef	RETRIES
#define	RETRIES	3
#endif

extern	char	*l64a ();
extern	char	*crypt ();
extern	char	*pw_encrypt ();
extern	int	errno;
extern	long	a64l ();
extern	void	entry ();
extern	time_t	time ();
extern	char	*malloc ();
extern	char	*getpass ();
#ifdef	NDBM
extern	int	sg_dbm_mode;
extern	int	gr_dbm_mode;
#endif

/*
 * usage - display usage message
 */

void
usage ()
{
	fprintf (stderr, "usage: %s [ -r|R ] group\n", Prog);
	fprintf (stderr, "       %s [ -a user ] group\n", Prog);
	fprintf (stderr, "       %s [ -d user ] group\n", Prog);
#ifdef	SHADOWGRP
	fprintf (stderr, "       %s [ -A user[,user] ][ -M user[,user] group\n",
		Prog);
#else
	fprintf (stderr, "       %s [ -M user[,user] group\n", Prog);
#endif
	exit (1);
}

/*
 * add_list - add a member to a list of group members
 *
 *	the array of member names is searched for the new member
 *	name, and if not present it is added to a freshly allocated
 *	list of users.
 */

char **
add_list (list, member)
char	**list;
char	*member;
{
	int	i;
	char	**tmp;

	for (i = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member) == 0)
			return list;

	if (! (tmp = (char **) malloc ((i + 2) * sizeof member)))
		return 0;

	for (i = 0;list[i] != (char *) 0;i++)
		tmp[i] = list[i];

	tmp[i++] = strdup (member);
	tmp[i] = (char *) 0;

	return tmp;
}

/*
 * del_list - delete a group member from a list of members
 *
 *	del_list searches a list of group members, copying the
 *	members which do not match "member" to a newly allocated
 *	list.
 */

char **
del_list (list, member)
char	**list;
char	*member;
{
	int	i, j;
	char	**tmp;

	for (j = i = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member))
			j++;

	tmp = (char **) malloc ((j + 1) * sizeof member);

	for (j = i = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member) != 0)
			tmp[j++] = list[i];

	tmp[j] = (char *) 0;

	return tmp;
}

/*
 * comma_to_list - convert comma-separated list to (char *) array
 */

char **
comma_to_list (comma)
char	*comma;
{
	char	*members;
	char	**array;
	int	i;
	char	*cp, *cp2;

	/*
	 * Make a copy since we are going to be modifying the list
	 */

	if (! (members = strdup (comma))) {
		perror ("malloc");
		exit (1);
	}

	/*
	 * Count the number of commas in the list
	 */

	for (cp = members, i = 0;;i++)
		if (cp2 = strchr (cp, ','))
			cp = cp2 + 1;
		else
			break;

	/*
	 * Add 2 - one for the ending NULL, the other for the last item
	 */

	i += 2;

	/*
	 * Allocate the array we're going to store the pointers into.
	 */

	if (! (array = (char **) malloc (sizeof (char *) * i))) {

		/*
		 * Can't happen ...
		 */

		perror ("malloc");
		exit (1);
	}

	/*
	 * Now go walk that list all over again, this time building the
	 * array of pointers.
	 */

	for (cp = members, i = 0;;i++) {
		array[i] = cp;
		if (cp2 = strchr (cp, ',')) {
			*cp2++ = '\0';
			cp = cp2;
		} else {
			array[i + 1] = (char *) 0;
			break;
		}
	}

	/*
	 * Return the new array of pointers
	 */

	return array;
}

/*
 * check_list - check a comma-separated list of user names for validity
 *
 *	check_list scans a comma-separated list of user names and checks
 *	that each listed name exists.
 */

int
check_list (users)
char	*users;
{
	char	*cp;
	char	*end;
	char	*start;
	char	user[16];
	int	errors = 0;
	int	len;

	for (start = users;start && *start;start = end) {
		if (end = strchr (start, ',')) {
			if ((len = end - start) > 15)
				len = 15;

			strncpy (user, start, len);
			user[len] = 0;
			end++;
		} else {
			if ((len = strlen (start)) > 15)
				len = 15;

			strncpy (user, start, len);
			user[len] = 0;
		}

		/*
		 * This user must exist.
		 */

		if (! getpwnam (user)) {
			fprintf (stderr, "%s: unknown user %s\n", Prog, user);
			errors++;
		}
	}
	return errors;
}

/*
 * gpasswd - administer the /etc/group file
 *
 *	-a user		add user to the named group
 *	-d user		remove user from the named group
 *	-r		remove password from the named group
 *	-R		restrict access to the named group
 *	-A user,...	make list of users the administrative users
 *	-M user,...	make list of users the group members
 */

int
main (argc, argv)
int	argc;
char	**argv;
{
	extern	int	optind;
	extern	char	*optarg;
	int	flag;
	int	i;
	void	die ();
	char	*cp;
	char	*getlogin ();
	char	*getpass ();
	int	amroot;
	int	retries;
	int	ruid = getuid();
	struct	group	*gr = 0;
	struct	group	*getgrnam ();
	struct	group	*sgetgrent ();
#ifdef	SHADOWGRP
	struct	sgrp	*sg = 0;
	struct	sgrp	sgent;
	struct	sgrp	*getsgnam ();
#endif
	struct	passwd	*pw = 0;
	struct	passwd	*getpwuid ();
	struct	passwd	*getpwnam ();

	/*
	 * Make a note of whether or not this command was invoked
	 * by root.  This will be used to bypass certain checks
	 * later on.  Also, set the real user ID to match the
	 * effective user ID.  This will prevent the invoker from
	 * issuing signals which would interfer with this command.
	 */

	amroot = getuid () == 0;
#ifdef	NDBM
	sg_dbm_mode = O_RDWR;
	gr_dbm_mode = O_RDWR;
#endif
	setuid (geteuid ());

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif
	setbuf (stdout, (char *) 0);
	setbuf (stderr, (char *) 0);

	while ((flag = getopt (argc, argv, "a:d:grRA:M:")) != EOF) {
		switch (flag) {
			case 'a':	/* add a user */
				aflg++;
				user = optarg;
				break;
			case 'A':
				Aflg++;
				admins = optarg;
				break;
			case 'd':	/* delete a user */
				dflg++;
				user = optarg;
				break;
			case 'g':	/* no-op from normal password */
				break;
			case 'M':
				Mflg++;
				members = optarg;
				break;
			case 'r':	/* remove group password */
				rflg++;
				break;
			case 'R':	/* restrict group password */
				Rflg++;
				break;
			default:
				usage ();
		}
	}

	/*
	 * Make sure exclusive flags are exclusive
	 */

	if (aflg + dflg + rflg + Rflg + (Aflg || Mflg) > 1)
		usage ();

	/*
	 * If the password is being changed, the input and output must
	 * both be a tty.  The typical keyboard signals are caught
	 * so the termio modes can be restored.
	 */

	if (! aflg && ! dflg && ! rflg && ! Rflg && ! Aflg && ! Mflg) {
		if (! isatty (0) || ! isatty (1))
			exit (1);

		die (0);			/* save tty modes */

		signal (SIGHUP, die);
		signal (SIGINT, die);
		signal (SIGQUIT, die);
		signal (SIGTERM, die);
#ifdef	SIGTSTP
		signal (SIGTSTP, die);
#endif
	}

	/*
	 * Determine the name of the user that invoked this command.
	 * This is really hit or miss because there are so many ways
	 * that command can be executed and so many ways to trip up
	 * the routines that report the user name.
	 */

	if ((cp = getlogin ()) && (pw = getpwnam (cp)) && pw->pw_uid == ruid) {
					/* need user name */
		(void) strcpy (name, cp);
	} else if (pw = getpwuid (ruid)) /* get it from password file */
		strcpy (name, pw->pw_name);
	else {				/* can't find user name! */
		fprintf (stderr, "Who are you?\n");
		exit (1);
	}
	if (! (pw = getpwnam (name)))
		goto failure;		/* can't get my name ... */
		
	/*
	 * Get the name of the group that is being affected.  The group
	 * entry will be completely replicated so it may be modified
	 * later on.
	 */

	if (! (group = argv[optind]))
		usage ();

	if (! (gr = getgrnam (group))) {
		fprintf (stderr, "unknown group: %s\n", group);
		exit (1);
	}
	grent = *gr;
	grent.gr_name = strdup (gr->gr_name);
	grent.gr_passwd = strdup (gr->gr_passwd);

	for (i = 0;gr->gr_mem[i];i++)
		;
	grent.gr_mem = (char **) malloc ((i + 1) * sizeof (char *));
	for (i = 0;gr->gr_mem[i];i++)
		grent.gr_mem[i] = strdup (gr->gr_mem[i]);
	grent.gr_mem[i] = (char *) 0;
#ifdef	SHADOWGRP
	if (sg = getsgnam (group)) {
		sgent = *sg;
		sgent.sg_name = strdup (sg->sg_name);
		sgent.sg_passwd = strdup (sg->sg_passwd);

		for (i = 0;sg->sg_mem[i];i++)
			;
		sgent.sg_mem = (char **) malloc (sizeof (char *) * (i + 1));
		for (i = 0;sg->sg_mem[i];i++)
			sgent.sg_mem[i] = strdup (sg->sg_mem[i]);
		sgent.sg_mem[i] = 0;

		for (i = 0;sg->sg_adm[i];i++)
			;
		sgent.sg_adm = (char **) malloc (sizeof (char *) * (i + 1));
		for (i = 0;sg->sg_adm[i];i++)
			sgent.sg_adm[i] = strdup (sg->sg_adm[i]);
		sgent.sg_adm[i] = 0;
	} else {
		sgent.sg_name = strdup (group);
		sgent.sg_passwd = grent.gr_passwd;
		grent.gr_passwd = "!";

		for (i = 0;grent.gr_mem[i];i++)
			;
		sgent.sg_mem = (char **) malloc (sizeof (char *) * (i + 1));
		for (i = 0;grent.gr_mem[i];i++)
			sgent.sg_mem[i] = strdup (grent.gr_mem[i]);
		sgent.sg_mem[i] = 0;

		sgent.sg_adm = (char **) malloc (sizeof (char *) * 2);
		if (sgent.sg_mem[0]) {
			sgent.sg_adm[0] = strdup (sgent.sg_mem[0]);
			sgent.sg_adm[1] = 0;
		} else
			sgent.sg_adm[0] = 0;

		sg = &sgent;
	}
#endif

#ifdef	SHADOWGRP

	/*
	 * The policy here for changing a group is that 1) you must be
	 * root or 2). you must be listed as an administrative member.
	 * Administrative members can do anything to a group that the
	 * root user can.
	 */

	if (! amroot) {
		for (i = 0;sgent.sg_adm[i];i++)
			if (strcmp (sgent.sg_adm[i], name) == 0)
				break;

		if (sgent.sg_adm[i] == (char *) 0)
			goto failure;
	}
#else

	/*
	 * The policy here for changing a group is that 1) you must bes
	 * root or 2) you must be the first listed member of the group.
	 * The first listed member of a group can do anything to that
	 * group that the root user can.  The rationale for this hack is
	 * that the FIRST user is probably the most important user in
	 * this entire group.
	 */

	if (! amroot) {
		if (grent.gr_mem[0] == (char *) 0)
			goto failure;

		if (strcmp (grent.gr_mem[0], name) != 0)
			goto failure;
	}

#endif	/* SHADOWGRP */

	/*
	 * Removing a password is straight forward.  Just set the
	 * password field to a "".
	 */

	if (rflg) {
		grent.gr_passwd = "";
#ifdef	SHADOWGRP
		sgent.sg_passwd = "";
#endif
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "remove password from group %s\n", group);
#endif
		goto output;
	} else if (Rflg) {

	/*
	 * Same thing for restricting the group.  Set the password
	 * field to "!".
	 */

		grent.gr_passwd = "!";
#ifdef	SHADOWGRP
		sgent.sg_passwd = "!";
#endif
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "restrict access to group %s\n", group);
#endif
		goto output;
	}

	/*
	 * Adding a member to a member list is pretty straightforward
	 * as well.  Call the appropriate routine and split.
	 */

	if (aflg) {
		if (getpwnam (user) == (struct passwd *) 0) {
			fprintf (stderr, "%s: unknown user %s\n", Prog, user);
			exit (1);
		}
		printf ("Adding user %s to group %s\n", user, group);
		grent.gr_mem = add_list (grent.gr_mem, user);
#ifdef	SHADOWGRP
		sgent.sg_mem = add_list (sgent.sg_mem, user);
#endif
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "add member %s to group %s\n", user, group);
#endif
		goto output;
	}

	/*
	 * Removing a member from the member list is the same deal
	 * as adding one, except the routine is different.
	 */

	if (dflg) {
		int	removed = 0;

		for (i = 0;grent.gr_mem[i];i++)
			if (strcmp (user, grent.gr_mem[i]) == 0)
				break;

		printf ("Removing user %s from group %s\n", user, group);

		if (grent.gr_mem[i] != (char *) 0) {
			removed = 1;
			grent.gr_mem = del_list (grent.gr_mem, user);
		}
#ifdef	SHADOWGRP
		for (i = 0;sgent.sg_mem[i];i++)
			if (strcmp (user, sgent.sg_mem[i]) == 0)
				break;

		if (sgent.sg_mem[i] != (char *) 0) {
			removed = 1;
			sgent.sg_mem = del_list (sgent.sg_mem, user);
		}
#endif
		if (! removed) {
			fprintf (stderr, "%s: unknown member %s\n", Prog, user);
			exit (1);
		}
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "remove member %s from group %s\n",
				user, group);
#endif
		goto output;
	}

	/*
	 * Replacing the entire list of members is simple.  Check the list
	 * to make sure everyone is a real user.  Then slap the new list
	 * in place.
	 */

	if (Mflg) {

		/*
		 * Only root can replace the entire list.
		 */

		if (! amroot)
			goto failure;

		/*
		 * Check the list for validity, then put it in.
		 */

		if (check_list (members))
			exit (1);

#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "set members of %s to %s\n", group, members);
#endif
#ifdef	SHADOWGRP
		sgent.sg_mem = comma_to_list (members);
		grent.gr_mem = comma_to_list (members);

		if (! Aflg)
			goto output;
#else
		grent.gr_mem = comma_to_list (members);

		goto output;
#endif
	}

#ifdef	SHADOWGRP

	/*
	 * Replacing the entire list of administators is simple.  Check the
	 * list to make sure everyone is a real user.  Then slap the new
	 * list in place.
	 */

	if (Aflg) {

		/*
		 * Only root can replace the entire list.
		 */

		if (! amroot)
			goto failure;

		/*
		 * Check the list for validity, then put it in.
		 */

		if (check_list (admins))
			exit (1);

#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "set administrators of %s to %s\n",
				group, members);
#endif
		sgent.sg_adm = comma_to_list (admins);

		goto output;
	}
#endif

	/*
	 * A new password is to be entered and it must be encrypted,
	 * etc.  The password will be prompted for twice, and both
	 * entries must be identical.  There is no need to validate
	 * the old password since the invoker is either the group
	 * owner, or root.
	 */

	printf ("Changing the password for group %s\n", group);

	for (retries = 0;retries < RETRIES;retries++) {
		if (! (cp = getpass ("New Password:")))
			exit (1);
		else {
			strcpy (pass, cp);
			bzero (cp, strlen (cp));
		}
		if (! (cp = getpass ("Re-enter new password:")))
			exit (1);
		else {
			strcpy (pass2, cp);
			bzero (cp, strlen (cp));
		}
		if (strcmp (pass, pass2) == 0)
			break;

		bzero (pass, sizeof pass);
		bzero (pass2, sizeof pass2);

		if (retries + 1 < RETRIES)
			puts ("They don't match; try again");
	}
	bzero (pass2, sizeof pass2);

	if (retries == RETRIES) {
		fprintf (stderr, "%s: Try again later\n", Prog);
		exit (1);
	}
#ifdef	SHADOWGRP
	sgent.sg_passwd = pw_encrypt (pass, (char *) 0);
#else
	grent.gr_passwd = pw_encrypt (pass, (char *) 0);
#endif
	bzero (pass, sizeof pass);
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, "change the password for group %s\n", group);
#endif

	/*
	 * This is the common arrival point to output the new group
	 * file.  The freshly crafted entry is in allocated space.
	 * The group file will be locked and opened for writing.  The
	 * new entry will be output, etc.
	 */

output:
	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif

	if (! gr_lock ()) {
		fprintf (stderr, "%s: can't get lock\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "failed to get lock for /etc/group\n");
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_lock ()) {
		fprintf (stderr, "%s: can't get shadow lock\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "failed to get lock for /etc/gshadow\n");
#endif
		exit (1);
	}
#endif
	if (! gr_open (O_RDWR)) {
		fprintf (stderr, "%s: can't open file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot open /etc/group\n");
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_open (O_RDWR)) {
		fprintf (stderr, "%s: can't open shadow file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot open /etc/gshadow\n");
#endif
		exit (1);
	}
#endif
	if (! gr_update (&grent)) {
		fprintf (stderr, "%s: can't update entry\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot update /etc/group\n");
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_update (&sgent)) {
		fprintf (stderr, "%s: can't update shadow entry\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot update /etc/gshadow\n");
#endif
		exit (1);
	}
#endif
	if (! gr_close ()) {
		fprintf (stderr, "%s: can't re-write file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot re-write /etc/group\n");
#endif
		exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_close ()) {
		fprintf (stderr, "%s: can't re-write shadow file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot re-write /etc/gshadow\n");
#endif
		exit (1);
	}
	(void) sgr_unlock ();
#endif
	if (! gr_unlock ()) {
		fprintf (stderr, "%s: can't unlock file\n", Prog);
		exit (1);
	}
#ifdef	NDBM
	if (access ("/etc/group.pag", 0) == 0 && ! gr_dbm_update (&grent)) {
		fprintf (stderr, "%s: can't update DBM files\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot update /etc/group DBM files\n");
#endif
		exit (1);
	}
	endgrent ();
#ifdef	SHADOWGRP
	if (access ("/etc/gshadow.pag", 0) == 0 && ! sg_dbm_update (&sgent)) {
		fprintf (stderr, "%s: can't update DBM shadow files\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_WARN, "cannot update /etc/gshadow DBM files\n");
#endif
		exit (1);
	}
	endsgent ();
#endif
#endif
	exit (0);
	/*NOTREACHED*/

failure:
	fprintf (stderr, "Permission denied.\n");
	exit (1);
	/*NOTREACHED*/
}

/*
 * die - set or reset termio modes.
 *
 *	die() is called before processing begins.  signal() is then
 *	called with die() as the signal handler.  If signal later
 *	calls die() with a signal number, the terminal modes are
 *	then reset.
 */

void
die (killed)
int	killed;
{
#if defined(BSD) || defined(SUN)
	static	struct	sgtty	sgtty;

	if (killed)
		stty (0, &sgtty);
	else
		gtty (0, &sgtty);
#else
#if defined(SVR4) || defined (SUN4)
	static	struct	termios	sgtty;

	if (killed)
		tcsetattr (0, TCSANOW, &sgtty);
	else
		tcgetattr (0, &sgtty);
#else	/* !SVR4 */
	static	struct	termio	sgtty;

	if (killed)
		ioctl (0, TCSETA, &sgtty);
	else
		ioctl (0, TCGETA, &sgtty);
#endif	/* SVR4 */
#endif	/* BSD || SUN */
	if (killed) {
		putchar ('\n');
		fflush (stdout);
		exit (killed);
	}
}
