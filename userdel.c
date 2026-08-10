/*
 * Copyright 1991, 1992, 1993, John F. Haugh II
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

#ifndef lint
static	char	sccsid[] = "@(#)userdel.c	3.14	08:06:43	07 May 1993";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include "pwd.h"
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <utmp.h>

#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "config.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#endif
#include "pwauth.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN LOG_WARNING
#endif
#endif

#ifndef	NGROUPS_MAX
#define	NGROUPS_MAX	64
#endif

#if defined(DIR_XENIX) || defined(DIR_BSD) || defined(DIR_SYSV)
#define	DIR_ANY
#endif

char	user_name[BUFSIZ];
uid_t	user_id;
char	user_home[BUFSIZ];

char	*Prog;
#ifdef	DIR_ANY
int	rflg;
#endif

#ifdef	NDBM
extern	int	pw_dbm_mode;
#ifdef	SHADOWPWD
extern	int	sp_dbm_mode;
#endif
extern	int	gr_dbm_mode;
#ifdef	SHADOWGRP
extern	int	sg_dbm_mode;
#endif
#endif
extern	struct	group	*getgrnam();
extern	struct	group	*getgrgid();
extern	struct	group	*gr_next();
extern	struct	passwd	*getpwnam();
extern	struct	passwd	*pw_next();
extern	struct	passwd	*pw_locate();

#ifdef	SHADOWPWD
extern	int	spw_lock();
extern	int	spw_unlock();
extern	int	spw_open();
extern	int	spw_close();
extern	struct	spwd	*spw_locate();
#endif

#ifdef	SHADOWGRP
extern	int	sgr_lock();
extern	int	sgr_unlock();
extern	int	sgr_open();
extern	int	sgr_close();
extern	struct	sgrp	*sgr_next();
#endif

extern	char	*malloc();

/*
 * del_list - delete a member from a list of group members
 *
 *	the array of member names is searched for the old member
 *	name, and if present it is deleted from a freshly allocated
 *	list of users.
 */

char **
del_list (list, member)
char	**list;
char	*member;
{
	int	i, j;
	char	**tmp;

	/*
	 * Scan the list for the new name.  Return the original list
	 * pointer if it is present.
	 */

	for (i = j = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member))
			j++;

	if (j == i)
		return list;

	/*
	 * Allocate a new list pointer large enough to hold all the
	 * old entries, and the new entries as well.
	 */

	if (! (tmp = (char **) malloc ((j + 2) * sizeof member)))
		return 0;

	/*
	 * Copy the original list to the new list, then append the
	 * new member and NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = j = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member))
			tmp[j++] = list[i];

	tmp[j] = (char *) 0;

	return tmp;
}

/*
 * usage - display usage message and exit
 */

usage ()
{
#ifdef	DIR_ANY
	fprintf (stderr, "usage: %s [-r] name\n", Prog);
#else
	fprintf (stderr, "usage: %s name\n", Prog);
#endif
	exit (2);
}

/*
 * update_groups - delete user from secondary group set
 *
 *	update_groups() takes the user name that was given and searches
 *	the group files for membership in any group.
 */

void
update_groups ()
{
	int	i;
	struct	group	*grp;
#ifdef	SHADOWGRP
	struct	sgrp	*sgrp;
#endif	/* SHADOWGRP */

	/*
	 * Scan through the entire group file looking for the groups that
	 * the user is a member of.
	 */

	for (gr_rewind (), grp = gr_next ();grp;grp = gr_next ()) {

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */

		for (i = 0;grp->gr_mem[i];i++)
			if (strcmp (grp->gr_mem[i], user_name) == 0)
				break;

		if (grp->gr_mem[i] == (char *) 0)
			continue;

		/* 
		 * Delete the username from the list of group members and
		 * update the group entry to reflect the change.
		 */

		grp->gr_mem = del_list (grp->gr_mem, user_name);
		if (! gr_update (grp))
			fprintf (stderr, "%s: error updating group entry\n",
				Prog);

		/*
		 * Update the DBM group file with the new entry as well.
		 */

#ifdef	NDBM
		if (! gr_dbm_update (grp))
			fprintf (stderr, "%s: cannot update dbm group entry\n",
				Prog);
#endif	/* NDBM */
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "delete `%s' from group `%s'\n",
			user_name, grp->gr_name);
#endif	/* USE_SYSLOG */
	}
#ifdef	NDBM
	endgrent ();
#endif	/* NDBM */
#ifdef	SHADOWGRP
	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of.  Both the administrative list and
	 * the ordinary membership list is checked.
	 */

	for (sgr_rewind (), sgrp = sgr_next ();sgrp;sgrp = sgr_next ()) {
		int	group_changed = 0;

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */

		for (i = 0;sgrp->sg_mem[i];i++)
			if (strcmp (sgrp->sg_mem[i], user_name) == 0)
				break;

		if (sgrp->sg_mem[i]) {
			sgrp->sg_mem = del_list (sgrp->sg_mem, user_name);
			group_changed = 1;
		}
		for (i = 0;sgrp->sg_adm[i];i++) {
			if (strcmp (sgrp->sg_adm[i], user_name) == 0)
				break;
		}
		if (sgrp->sg_adm[i]) {
			sgrp->sg_adm = del_list (sgrp->sg_adm, user_name);
			group_changed = 1;
		}
		if (! group_changed)
			continue;

		if (! sgr_update (sgrp))
			fprintf (stderr, "%s: error updating group entry\n",
				Prog);
#ifdef	NDBM
		/*
		 * Update the DBM group file with the new entry as well.
		 */

		if (! sg_dbm_update (sgrp))
			fprintf (stderr, "%s: cannot update dbm group entry\n",
				Prog);
#endif	/* NDBM */
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "delete `%s' from shadow group `%s'\n",
			user_name, sgrp->sg_name);
#endif	/* USE_SYSLOG */
	}
#ifdef	NDBM
	endsgent ();
#endif	/* NDBM */
#endif	/* SHADOWGRP */
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new user.  This causes any modified entries to be written out.
 */

close_files ()
{
	if (! pw_close ())
		fprintf (stderr, "%s: cannot rewrite password file\n", Prog);
#ifdef	SHADOWPWD
	if (! spw_close ())
		fprintf (stderr, "%s: cannot rewrite shadow password file\n",	
			Prog);
#endif
	if (! gr_close ())
		fprintf (stderr, "%s: cannot rewrite group file\n",
			Prog);

	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (! sgr_close ())
		fprintf (stderr, "%s: cannot rewrite shadow group file\n",
			Prog);

	(void) sgr_unlock ();
#endif
#ifdef	SHADOWPWD
	(void) spw_unlock ();
#endif
	(void) pw_unlock ();
}

/*
 * open_files - lock and open the password files
 *
 *	open_files() opens the two password files.
 */

open_files ()
{
	if (! pw_lock ()) {
		fprintf (stderr, "%s: unable to lock password file\n", Prog);
		exit (1);
	}
	if (! pw_open (O_RDWR)) {
		fprintf (stderr, "%s: unable to open password file\n", Prog);
		fail_exit (1);
	}
#ifdef	SHADOWPWD
	if (! spw_lock ()) {
		fprintf (stderr, "%s: cannot lock shadow password file\n", Prog);
		fail_exit (1);
	}
	if (! spw_open (O_RDWR)) {
		fprintf (stderr, "%s: cannot open shadow password file\n", Prog);
		fail_exit (1);
	}
#endif
	if (! gr_lock ()) {
		fprintf (stderr, "%s: unable to lock group file\n", Prog);
		fail_exit (1);
	}
	if (! gr_open (O_RDWR)) {
		fprintf (stderr, "%s: cannot open group file\n", Prog);
		fail_exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_lock ()) {
		fprintf (stderr, "%s: unable to lock shadow group file\n", Prog);
		fail_exit (1);
	}
	if (! sgr_open (O_RDWR)) {
		fprintf (stderr, "%s: cannot open shadow group file\n", Prog);
		fail_exit (1);
	}
#endif
}

/*
 * update_user - delete the user entries
 *
 *	update_user() deletes the password file entries for this user
 *	and will update the group entries as required.
 */

update_user ()
{
	struct	passwd	*pwd;
#ifdef	SHADOWPWD
	struct	spwd	*spwd;

	if ((spwd = spw_locate (user_name)) && spwd->sp_pwdp[0] == '@') {
		if (pw_auth (spwd->sp_pwdp + 1, user_name, PW_DELETE)) {
#ifdef	USE_SYSLOG
			syslog (LOG_ERR,
				"failed deleting auth `%s' for user `%s'\n",
				spwd->sp_pwdp + 1, user_name);
#endif 	/* USE_SYSLOG */
			fprintf (stderr,
				"%s: error deleting authentication\n",
			Prog);
		}
#ifdef	USE_SYSLOG
		else {
			syslog (LOG_INFO,
				"delete auth `%s' for user `%s'\n",
				spwd->sp_pwdp + 1, user_name);
		}
#endif	/* USE_SYSLOG */
	}
#endif	/* SHADOWPWD */
	if ((pwd = pw_locate (user_name)) && pwd->pw_passwd[0] == '@') {
		if (pw_auth (pwd->pw_passwd + 1, user_name, PW_DELETE)) {
#ifdef	USE_SYSLOG
			syslog (LOG_ERR,
				"failed deleting auth `%s' for user `%s'\n",
				pwd->pw_passwd + 1, user_name);
#endif 	/* USE_SYSLOG */
			fprintf (stderr, "%s: error deleting authentication\n",
				Prog);
		}
#ifdef	USE_SYSLOG
		else {
			syslog (LOG_INFO,
				"delete auth `%s' for user `%s'\n",
				pwd->pw_passwd + 1, user_name);
		}
#endif	/* USE_SYSLOG */
	}
	if (! pw_remove (user_name))
		fprintf (stderr, "%s: error deleting password entry\n", Prog);
#ifdef	SHADOWPWD
	if (! spw_remove (user_name))
		fprintf (stderr, "%s: error deleting shadow password entry\n",
			Prog);
#endif
#if defined(DBM) || defined(NDBM)
	if (access ("/etc/passwd.pag", 0) == 0) {
		if ((pwd = getpwnam (user_name)) && ! pw_dbm_remove (pwd))
			fprintf (stderr,
				"%s: error deleting password dbm entry\n",
				Prog);
	}

	/*
	 * If the user's UID is a duplicate the duplicated entry needs
	 * to be updated so that a UID match can be found in the DBM
	 * files.
	 */

	for (pw_rewind (), pwd = pw_next ();pwd;pwd = pw_next ()) {
		if (pwd->pw_uid == user_id) {
			pw_dbm_update (pwd);
			break;
		}
	}
#endif
#if defined(NDBM) && defined(SHADOWPWD)
	if (access ("/etc/shadow.pag", 0) == 0 && ! sp_dbm_remove (user_name))
		fprintf (stderr, "%s: error deleting shadow passwd dbm entry\n",
			Prog);

	endspent ();
#endif
#if defined(DBM) || defined(NDBM)
	endpwent ();
#endif
#ifdef	USE_SYSLOG
	syslog (LOG_INFO, "delete user `%s'\n", user_name);
#endif
}

/*
 * fail_exit - exit with a failure code after unlocking the files
 */

fail_exit (code)
int	code;
{
	(void) pw_unlock ();
	(void) gr_unlock ();
#ifdef	SHADOWPWD
	(void) spw_unlock ();
#endif
#ifdef	SHADOWGRP
	(void) sgr_unlock ();
#endif
	exit (code);
}

/*
 * user_busy - see if user is logged in.
 */

user_busy (name, uid)
char	*name;
uid_t	uid;
{
	struct	utmp	*utent;

	/*
	 * We see if the user is logged in by looking for the user name
	 * in the utmp file.
	 */

	setutent ();

	while (utent = getutent ()) {
#ifdef	USG
		if (utent->ut_type != USER_PROCESS)
			continue;
#else
		if (utent->ut_user[0] == '\0')
			continue;
#endif
		if (strncmp (utent->ut_user, name, sizeof utent->ut_user))
			continue;

		fprintf (stderr, "%s: user %s is currently logged in.\n",
			Prog, name);
		exit (1);
	}
}

/* 
 * user_cancel - cancel cron and at jobs
 *
 *	user_cancel removes the crontab and any at jobs for a user
 */

user_cancel (user)
char	*user;
{
	char	buf[BUFSIZ];

#ifdef	HAS_CRONTAB

	/* 
	 * Remove the crontab if there is one.
	 */

	sprintf (buf, "/bin/crontab -r -u %s", user);
	system (buf);
#endif
#ifdef	HAS_ATRM

	/*
	 * Remove any at jobs as well.
	 */

	sprintf (buf, "/bin/atrm -f %s", user);
	system (buf);
#endif
}

/*
 * main - useradd command
 */

main (argc, argv)
int	argc;
char	**argv;
{
	struct	passwd	*pwd;
	int	arg;
	int	errors = 0;
	extern	int	optind;
	extern	char	*optarg;

	/*
	 * Get my name so that I can use it to report errors.
	 */

	if (Prog = strrchr (argv[0], '/'))
		Prog++;
	else
		Prog = argv[0];

#ifdef	USE_SYSLOG
	openlog (Prog, LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
#endif

	/*
	 * The open routines for the DBM files don't use read-write
	 * as the mode, so we have to clue them in.
	 */

#if defined(DBM) || defined(NDBM)
	pw_dbm_mode = O_RDWR;
#endif
#ifdef	NDBM
#ifdef	SHADOWPWD
	sp_dbm_mode = O_RDWR;
#endif
	gr_dbm_mode = O_RDWR;
#ifdef	SHADOWGRP
	sg_dbm_mode = O_RDWR;
#endif
#endif
	while ((arg = getopt (argc, argv, "r")) != EOF)
#ifdef	DIR_ANY
		if (arg != 'r')
			usage ();
		else
			rflg++;
#else
		usage ();
#endif
	
	if (optind == argc)
		usage ();

	/*
	 * Start with a quick check to see if the user exists.
	 */

	strncpy (user_name, argv[argc - 1], BUFSIZ);

	if (! (pwd = getpwnam (user_name))) {
		fprintf (stderr, "%s: user %s does not exist\n",
			Prog, user_name);
		exit (6);
	}
	user_id = pwd->pw_uid;
	strcpy (user_home, pwd->pw_dir);

	/*
	 * Check to make certain the user isn't logged in.
	 */

	user_busy (user_name, user_id);

	/*
	 * Do the hard stuff - open the files, create the user entries,
	 * create the home directory, then close and update the files.
	 */

	open_files ();

	update_user ();
	update_groups ();

#ifdef	DIR_ANY
	if (rflg) {
		if (remove_tree (user_home) || rmdir (user_home)) {
			fprintf (stderr, "%s: error removing directory %s\n",
				Prog, user_home);

			errors++;
		}
	}
#endif
	/*
	 * Cancel any crontabs or at jobs.  Have to do this before we
	 * remove the entry from /etc/passwd.
	 */

	user_cancel (user_name);

	close_files ();

	exit (errors ? 12:0);
	/*NOTREACHED*/
}
