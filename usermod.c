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
static	char	sccsid[] = "@(#)usermod.c	3.16	08:11:52	07 May 1993";
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

#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "config.h"
#ifdef	SHADOWPWD
#include "shadow.h"
#endif
#include "faillog.h"
#include "lastlog.h"
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

#if !defined(MDY_DATE) && !defined(DMY_DATE) && !defined(YMD_DATE)
#define	MDY_DATE	1
#endif
#if (defined (MDY_DATE) && (defined (DMY_DATE) || defined (YMD_DATE))) || \
    (defined (DMY_DATE) && (defined (MDY_DATE) || defined (YMD_DATE)))
Error: You must only define one of MDY_DATE, DMY_DATE, or YMD_DATE
#endif

#define	VALID(s)	(strcspn (s, ":\n") == strlen (s))

char	user_name[BUFSIZ];
char	user_newname[BUFSIZ];
char	user_auth[BUFSIZ];
char	user_newauth[BUFSIZ];
uid_t	user_id;
uid_t	user_newid;
gid_t	user_gid;
gid_t	user_newgid;
char	user_comment[BUFSIZ];
char	user_home[BUFSIZ];
char	user_newhome[BUFSIZ];
char	user_shell[BUFSIZ];
#ifdef	SHADOWPWD
long	user_expire;
long	user_inactive;
#endif
int	user_ngroups = -1;
gid_t	user_groups[NGROUPS_MAX];
struct	passwd	user_pwd;
#ifdef	SHADOWPWD
struct	spwd	user_spwd;
#endif

char	*Prog;
char	*auth_arg;

int	Aflg;	/* specify user defined authentication method                 */
int	uflg;	/* specify user ID for new account                            */
int	oflg;	/* permit non-unique user ID to be specified with -u          */
int	gflg;	/* primary group ID  for new account                          */
int	Gflg;	/* secondary group set for new account                        */
int	dflg;	/* home directory for new account                             */
int	sflg;	/* shell program for new account                              */
int	cflg;	/* comment (GECOS) field for new account                      */
int	mflg;	/* create user's home directory if it doesn't exist           */
int	fflg;	/* days until account with expired password is locked         */
int	eflg;	/* days after password changed before it becomes expired      */
int	lflg;	/* new user name for user                                     */

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
extern	FILE	*fopen();
extern	int	fclose();
extern	char	*malloc();
extern	char	*mktemp();

extern	struct	group	*getgrnam();
extern	struct	group	*getgrgid();
extern	struct	group	*gr_next();
extern	struct	group	*gr_locate();
extern	int	gr_lock();
extern	int	gr_unlock();
extern	int	gr_rewind();
extern	int	gr_open();

#ifdef	SHADOWGRP
extern	struct	sgrp	*sgr_next();
extern	int	sgr_lock();
extern	int	sgr_unlock();
extern	int	sgr_rewind();
extern	int	sgr_open();
#endif

extern	struct	passwd	*getpwnam();
extern	struct	passwd	*pw_next();
extern	struct	passwd	*pw_locate();
extern	int	pw_lock();
extern	int	pw_unlock();
extern	int	pw_rewind();
extern	int	pw_open();

#ifdef	SHADOWPWD
extern	int	spw_lock();
extern	int	spw_unlock();
extern	int	spw_open();
extern	struct	spwd	*spw_locate();
#endif

#define	DAY	(24L*3600L)
#define	WEEK	(7*DAY)

#ifdef	ITI_AGING
#define	SCALE	(1)
#else
#define	SCALE	(DAY)
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

#ifdef	NEED_RENAME
/*
 * rename - rename a file to another name
 *
 *	rename is provided for systems which do not include the rename()
 *	system call.
 */

int
rename (begin, end)
char	*begin;
char	*end;
{
	struct	stat	s1, s2;
	extern	int	errno;
	int	orig_err = errno;

	if (stat (begin, &s1))
		return -1;

	if (stat (end, &s2)) {
		errno = orig_err;
	} else {

		/*
		 * See if this is a cross-device link.  We do this to
		 * insure that the link below has a chance of working.
		 */

		if (s1.st_dev != s2.st_dev) {
			errno = EXDEV;
			return -1;
		}

		/*
		 * See if we can unlink the existing destination
		 * file.  If the unlink works the directory is writable,
		 * so there is no need here to figure that out.
		 */

		if (unlink (end))
			return -1;
	}

	/*
	 * Now just link the original name to the final name.  If there
	 * was no file previously, this link will fail if the target
	 * directory isn't writable.  The unlink will fail if the source
	 * directory isn't writable, but life stinks ...
	 */

	if (link (begin, end) || unlink (begin))
		return -1;

	return 0;
}
#endif

/*
 * strtoday - compute the number of days since 1970.
 *
 * the total number of days prior to the current date is
 * computed.  january 1, 1970 is used as the origin with
 * it having a day number of 0.
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

	total = (long) ((year - 1970) * 365L) + (((year + 1) - 1970) / 4);
	total += (long) juldays[month] + (month > 2 && (year % 4) == 0 ? 1:0);
	total += (long) day - 1;

	return total;
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

	/*
	 * Scan the list for the new name.  Return the original list
	 * pointer if it is present.
	 */

	for (i = 0;list[i] != (char *) 0;i++)
		if (strcmp (list[i], member) == 0)
			return list;

	/*
	 * Allocate a new list pointer large enough to hold all the
	 * old entries, and the new entries as well.
	 */

	if (! (tmp = (char **) malloc ((i + 2) * sizeof member)))
		return 0;

	/*
	 * Copy the original list to the new list, then append the
	 * new member and NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = 0;list[i] != (char *) 0;i++)
		tmp[i] = list[i];

	tmp[i++] = strdup (member);
	tmp[i] = (char *) 0;

	return tmp;
}

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
 * get_groups - convert a list of group names to an array of group IDs
 *
 *	get_groups() takes a comma-separated list of group names and
 *	converts it to an array of group ID values.  Any unknown group
 *	names are reported as errors.
 */

int
get_groups (list)
char	*list;
{
	char	*cp;
	struct	group	*grp;
	int	errors = 0;

	/*
	 * Initialize the list to be empty
	 */

	user_ngroups = 0;

	if (! *list)
		return 0;

	/*
	 * So long as there is some data to be converted, strip off
	 * each name and look it up.  A mix of numerical and string
	 * values for group identifiers is permitted.
	 */

	do {
		/*
		 * Strip off a single name from the list
		 */

		if (cp = strchr (list, ','))
			*cp++ = '\0';

		/*
		 * Names starting with digits are treated as numerical
		 * GID values, otherwise the string is looked up as is.
		 */

		if (isdigit (*list))
			grp = getgrgid (atoi (list));
		else
			grp = getgrnam (list);

		/*
		 * There must be a match, either by GID value or by
		 * string name.
		 */

		if (! grp) {
			fprintf (stderr, "%s: unknown group %s\n", Prog, list);
			errors++;
		}

		/*
		 * Add the GID value from the group file to the user's
		 * list of groups.
		 */

		user_groups[user_ngroups++] = grp->gr_gid;

		list = cp;
	} while (list);

	/*
	 * Any errors in finding group names are fatal
	 */

	if (errors)
		return -1;

	return 0;
}

/*
 * usage - display usage message and exit
 */

usage ()
{
	fprintf (stderr,
		"usage: %s [-u uid [-o]] [-g group] [-G group,...] \n", Prog);
#ifdef	SHADOWPWD
	fprintf (stderr,
		"\t\t[-d home [-m]] [-s shell] [-c comment] [-l new_name]\n");
#ifdef	MDY_DATE
	fprintf (stderr,
		"\t\t[-f inactive ] [-e expire mm/dd/yy ] name\n");
#endif
#ifdef	DMY_DATE
	fprintf (stderr,
		"\t\t[-f inactive ] [-e expire dd/mm/yy ] name\n");
#endif
#ifdef	YMD_DATE
	fprintf (stderr,
		"\t\t[-f inactive ] [-e expire yy/mm/dd ] name\n");
#endif
#else	/* !SHADOWPWD */
	fprintf (stderr,
		"\t\t[-d home [-m]] [-s shell] [-c comment] [-l new_name]\n");
	fprintf (stderr,
		"\t\t[ -A {DEFAULT|program},... ] name\n");
#endif	/* SHADOWPWD */
	exit (2);
}

/*
 * new_pwent - initialize the values in a password file entry
 *
 *	new_pwent() takes all of the values that have been entered and
 *	fills in a (struct passwd) with them.
 */

void
new_pwent (pwent)
struct	passwd	*pwent;
{
	if (lflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user name `%s' to `%s'\n",
			pwent->pw_name, user_newname);
#endif
		pwent->pw_name = strdup (user_newname);
	}
	if (uflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' UID from `%d' to `%d'\n",
			pwent->pw_name, pwent->pw_uid, user_newid);
#endif
		pwent->pw_uid = user_newid;
	}
	if (gflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' GID from `%d' to `%d'\n",
			pwent->pw_name, pwent->pw_gid, user_newgid);
#endif
		pwent->pw_gid = user_newgid;
	}
	if (cflg)
		pwent->pw_gecos = strdup (user_comment);

	if (dflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' home from `%s' to `%s'\n",
			pwent->pw_name, pwent->pw_dir, user_newhome);
#endif
		pwent->pw_dir = strdup (user_newhome);
	}
	if (sflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' shell from `%s' to `%s'\n",
			pwent->pw_name, pwent->pw_shell, user_shell);
#endif
		pwent->pw_shell = strdup (user_shell);
	}
}

#ifdef	SHADOWPWD
/*
 * new_spent - initialize the values in a shadow password file entry
 *
 *	new_spent() takes all of the values that have been entered and
 *	fills in a (struct spwd) with them.
 */

void
new_spent (spent)
struct	spwd	*spent;
{
	if (lflg)
		spent->sp_namp = strdup (user_newname);

	if (fflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' inactive from `%d' to `%d'\n",
			spent->sp_namp, spent->sp_inact, user_inactive);
#endif
		spent->sp_inact = user_inactive;
	}
	if (eflg) {
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "change user `%s' expiration from `%d' to `%d'\n",
			spent->sp_namp, spent->sp_expire, user_expire);
#endif
		spent->sp_expire = user_expire;
	}
}
#endif	/* SHADOWPWD */

/*
 * grp_update - add user to secondary group set
 *
 *	grp_update() takes the secondary group set given in user_groups
 *	and adds the user to each group given by that set.
 */

void
grp_update ()
{
	int	i;
	int	is_member;
	int	was_member;
	struct	group	*grp;
#ifdef	SHADOWGRP
	int	was_admin;
	struct	sgrp	*sgrp;
#endif

	/*
	 * Lock and open the group file.  This will load all of the group
	 * entries.
	 */

	if (! gr_lock ()) {
		fprintf (stderr, "%s: error locking group file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, "error locking group file");
#endif
		exit (1);
	}
	if (! gr_open (O_RDWR)) {
		fprintf (stderr, "%s: error opening group file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, "error opening group file");
#endif
		fail_exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_lock ()) {
		fprintf (stderr, "%s: error locking shadow group file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, "error locking shadow group file");
#endif
		fail_exit (1);
	}
	if (! sgr_open (O_RDWR)) {
		fprintf (stderr, "%s: error opening shadow group file\n", Prog);
#ifdef	USE_SYSLOG
		syslog (LOG_ERR, "error opening shadow group file");
#endif
		fail_exit (1);
	}
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

		for (i = 0;i < user_ngroups;i++)
			if (grp->gr_gid == user_groups[i])
				break;

		is_member = i == user_ngroups ? -1:i;

		for (i = 0;grp->gr_mem[i];i++)
			if (strcmp (user_name, grp->gr_mem[i]) == 0)
				break;

		was_member = grp->gr_mem[i] ? i:-1;

		if (is_member == -1 && was_member == -1)
			continue;

		if (was_member >= 0 && (! Gflg || is_member >= 0)) {
			if (lflg) {
				grp->gr_mem = del_list (grp->gr_mem,
					user_name);
				grp->gr_mem = add_list (grp->gr_mem,
					user_newname);
#ifdef	USE_SYSLOG
				syslog (LOG_INFO,
					"change `%s' to `%s' in group `%s'\n",
					user_name, user_newname, grp->gr_name);
#endif
			}
		} else if (was_member >= 0 && Gflg && is_member < 0) {
			grp->gr_mem = del_list (grp->gr_mem, user_name);
#ifdef	USE_SYSLOG
			syslog (LOG_INFO, "delete `%s' from group `%s'\n",
				user_name, grp->gr_name);
#endif
		} else if (was_member < 0 && Gflg && is_member >= 0) {
			grp->gr_mem = add_list (grp->gr_mem,
				lflg ? user_newname:user_name);
#ifdef	USE_SYSLOG
			syslog (LOG_INFO, "add `%s' to group `%s'\n",
				lflg ? user_newname:user_name, grp->gr_name);
#endif
		} else
			continue;

		if (! gr_update (grp)) {
			fprintf (stderr, "%s: error adding new group entry\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error adding group entry");
#endif
			fail_exit (1);
		}
#ifdef	NDBM
		/*
		 * Update the DBM group file with the new entry as well.
		 */

		if (! gr_dbm_update (grp)) {
			fprintf (stderr, "%s: cannot add new dbm group entry\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error adding dbm group entry");
#endif
			fail_exit (1);
		}
#endif	/* NDBM */
	}
#ifdef NDBM
	endgrent ();
#endif

#ifdef	SHADOWGRP
	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of.
	 */

	for (sgr_rewind (), sgrp = sgr_next ();sgrp;sgrp = sgr_next ()) {

		/*
		 * See if the user was a member of this group
		 */

		for (i = 0;sgrp->sg_mem[i];i++)
			if (strcmp (sgrp->sg_mem[i], user_name) == 0)
				break;

		was_member = sgrp->sg_mem[i] ? i:-1;

		/*
		 * See if the user was an administrator of this group
		 */

		for (i = 0;sgrp->sg_adm[i];i++)
			if (strcmp (sgrp->sg_adm[i], user_name) == 0)
				break;

		was_admin = sgrp->sg_adm[i] ? i:-1;

		/*
		 * See if the user specified this group as one of their
		 * concurrent groups.
		 */

		for (i = 0;i < user_ngroups;i++) {
			if (! (grp = gr_locate (sgrp->sg_name)))
				continue;

			if (grp->gr_gid == user_groups[i])
				break;
		}
		is_member = i == user_ngroups ? -1:i;

		if (is_member == -1 && was_admin == -1 && was_member == -1)
			continue;

		if (was_admin >= 0 && lflg) {
			sgrp->sg_adm = del_list (sgrp->sg_adm, user_name);
			sgrp->sg_adm = add_list (sgrp->sg_adm, user_newname);
#ifdef	USE_SYSLOG
			syslog (LOG_INFO, "change admin `%s' to `%s' in shadow group `%s'\n",
				user_name, user_newname, sgrp->sg_name);
#endif
		}
		if (was_member >= 0 && (! Gflg || is_member >= 0)) {
			if (lflg) {
				sgrp->sg_mem = del_list (sgrp->sg_mem,
					user_name);
				sgrp->sg_mem = add_list (sgrp->sg_mem,
					user_newname);
#ifdef	USE_SYSLOG
				syslog (LOG_INFO, "change `%s' to `%s' in shadow group `%s'\n",
					user_name, user_newname, sgrp->sg_name);
#endif
			}
		} else if (was_member >= 0 && Gflg && is_member < 0) {
			sgrp->sg_mem = del_list (sgrp->sg_mem, user_name);
#ifdef	USE_SYSLOG
			syslog (LOG_INFO,
				"delete `%s' from shadow group `%s'\n",
				user_name, sgrp->sg_name);
#endif
		} else if (was_member < 0 && Gflg && is_member >= 0) {
			sgrp->sg_mem = add_list (sgrp->sg_mem,
				lflg ? user_newname:user_name);
#ifdef	USE_SYSLOG
			syslog (LOG_INFO, "add `%s' to shadow group `%s'\n",
				lflg ? user_newname:user_name, grp->gr_name);
#endif
		} else
			continue;

		/* 
		 * Update the group entry to reflect the changes.
		 */

		if (! sgr_update (sgrp)) {
			fprintf (stderr, "%s: error adding new group entry\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error adding shadow group entry\n");
#endif
			fail_exit (1);
		}
#ifdef	NDBM
		/*
		 * Update the DBM group file with the new entry as well.
		 */

		if (! sg_dbm_update (sgrp)) {
			fprintf (stderr, "%s: cannot add new dbm group entry\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR,
				"error adding dbm shadow group entry\n");
#endif
			fail_exit (1);
		}
#endif	/* NDBM */
	}
#ifdef NDBM
	endsgent ();
#endif	/* NDBM */
#endif	/* SHADOWGRP */
}

/*
 * check_new_id - verify the new UID for uniqueness
 *
 *	check_new_id() insures that the new UID does not exist already.
 *	It does this by checking that the UID has changed and that there
 *	is no current entry for this user ID.
 */

int
check_new_id ()
{
	/*
	 * First, the easy stuff.  If the ID can be duplicated, or if
	 * the ID didn't really change, just return.  If the ID didn't
	 * change, turn off those flags.  No sense doing needless work.
	 */

	if (oflg)
		return 0;

	if (user_id == user_newid) {
		uflg = lflg = 0;
		return 0;
	}
	if (getpwuid (user_newid))
		return -1;

	return 0;
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
 * split_auths - break up comma list into (char *) array
 */

split_auths (list, array)
char	*list;
char	**array;
{
	char	*cp, *end;
	int	i = 0;

	for (cp = list;cp;cp = end) {
		if (end = strchr (cp, ';'))
			*end++ = '\0';

		array[i++] = cp;
	}
	array[i] = 0;
}

/*
 * update_auths - find list of methods to update
 */

update_auths (old, new, update)
char	*old;
char	*new;
char	*update;
{
	char	oldbuf[257], newbuf[257];
	char	*oldv[32], *newv[32], *updatev[32];
	int	i, j, k;

	strcpy (oldbuf, old);
	split_auths (oldbuf, oldv);

	strcpy (newbuf, new);
	split_auths (newbuf, newv);

	for (i = j = k = 0;oldv[i];i++) {
		for (j = 0;newv[j];j++)
			if (strcmp (oldv[i], newv[j]) != 0)
				break;

		if (newv[j] != (char *) 0)
			updatev[k++] = oldv[i];
	}
	updatev[k] = 0;

	update[0] = '\0';
	for (i = 0;updatev[i];i++) {
		if (i)
			strcat (update, ";");

		strcat (update, updatev[i]);
	}
}

/*
 * add_auths - find list of methods to add
 */

add_auths (old, new, add)
char	*old;
char	*new;
char	*add;
{
	char	oldbuf[257], newbuf[257];
	char	*oldv[32], *newv[32], *addv[32];
	int	i, j, k;

	strcpy (oldbuf, old);
	split_auths (oldbuf, oldv);

	strcpy (newbuf, new);
	split_auths (newbuf, newv);

	for (i = j = k = 0;newv[i];i++) {
		for (j = 0;oldv[j];j++)
			if (strcmp (oldv[i], newv[j]) == 0)
				break;

		if (oldv[j] == (char *) 0)
			addv[k++] = newv[i];
	}
	addv[k] = 0;

	add[0] = '\0';
	for (i = 0;addv[i];i++) {
		if (i)
			strcat (add, ";");

		strcat (add, addv[i]);
	}
}

/*
 * delete_auths - find list of methods to delete
 */

delete_auths (old, new, remove)
char	*old;
char	*new;
char	*remove;
{
	char	oldbuf[257], newbuf[257];
	char	*oldv[32], *newv[32], *removev[32];
	int	i, j, k;

	strcpy (oldbuf, old);
	split_auths (oldbuf, oldv);

	strcpy (newbuf, new);
	split_auths (newbuf, newv);

	for (i = j = k = 0;oldv[i];i++) {
		for (j = 0;newv[j];j++)
			if (strcmp (oldv[i], newv[j]) == 0)
				break;

		if (newv[j] == (char *) 0)
			removev[k++] = oldv[i];
	}
	removev[k] = 0;

	remove[0] = '\0';
	for (i = 0;removev[i];i++) {
		if (i)
			strcat (remove, ";");

		strcat (remove, removev[i]);
	}
}

/*
 * convert_auth - convert the argument list to a authentication list
 */

convert_auth (auths, oldauths, list)
char	*auths;
char	*oldauths;
char	*list;
{
	char	*cp, *end;
	char	*old;
	char	buf[257];

	/*
	 * Copy each method.  DEFAULT is replaced by an encrypted string
	 * if one can be found in the current authentication list.
	 */

	strcpy (buf, list);
	auths[0] = '\0';
	for (cp = buf;cp;cp = end) {
		if (auths[0])
			strcat (auths, ";");

		if (end = strchr (cp, ','))
			*end++ = '\0';

		if (strcmp (cp, "DEFAULT") == 0) {
			if (old = get_password (oldauths))
				strcat (auths, old);
			else
				strcat (auths, "!");
		} else {
			strcat (auths, "@");
			strcat (auths, cp);
		}
	}
}

/*
 * valid_auth - check authentication list for validity
 */

valid_auth (methods)
char	*methods;
{
	char	*cp, *end;
	char	buf[257];
	int	default_cnt = 0;

	/*
	 * Cursory checks, length and illegal characters
	 */

	if (strlen (methods) > 256)
		return 0;

	if (! VALID (methods))
		return 0;

	/*
	 * Pick each method apart and check it.
	 */

	strcpy (buf, methods);
	for (cp = buf;cp;cp = end) {
		if (end = strchr (cp, ','))
			*end++ = '\0';

		if (strcmp (cp, "DEFAULT") == 0) {
			if (default_cnt++ > 0)
				return 0;
		} else if (cp[0] != '/')
			return 0;
	}
	return 1;
}

/*
 * process_flags - perform command line argument setting
 *
 *	process_flags() interprets the command line arguments and sets
 *	the values that the user will be created with accordingly.  The
 *	values are checked for sanity.
 */

void
process_flags (argc, argv)
int	argc;
char	**argv;
{
	extern	int	optind;
	extern	char	*optarg;
	struct	group	*grp;
	struct	passwd	*pwd;
	struct	spwd	*spwd;
	long	l;
	int	anyflag = 0;
	int	arg;

	if (argc == 1 || argv[argc - 1][0] == '-')
		usage ();

	if (! (pwd = getpwnam (argv[argc - 1]))) {
		fprintf (stderr, "%s: user %s does not exist\n",
			Prog, argv[argc - 1]);
		exit (6);
	}
	strcpy (user_name, pwd->pw_name);
	user_id = pwd->pw_uid;
	user_gid = pwd->pw_gid;
	strcpy (user_comment, pwd->pw_gecos);
	strcpy (user_home, pwd->pw_dir);
	strcpy (user_shell, pwd->pw_shell);

#ifdef	SHADOWPWD
	if (spwd = getspnam (user_name)) {
		user_expire = spwd->sp_expire;
		user_inactive = spwd->sp_inact;
	}
#endif
#ifdef	SHADOWPWD
	while ((arg = getopt (argc, argv, "A:u:og:G:d:s:c:mf:e:l:")) != EOF)
#else
	while ((arg = getopt (argc, argv, "A:u:og:G:d:s:c:ml:")) != EOF)
#endif
	{
		switch (arg) {
			case 'A':
				if (! valid_auth (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				auth_arg = optarg;
				Aflg++;
				break;
			case 'c':
				if (! VALID (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				strncpy (user_comment, optarg, BUFSIZ);
				cflg++;
				break;
			case 'd':
				if (! VALID (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				dflg++;
				strncpy (user_newhome, optarg, BUFSIZ);
				break;
#ifdef	SHADOWPWD
			case 'e':
				l = strtoday (optarg);
#ifdef	ITI_AGING
				l *= DAY;
#endif
				user_expire = l;
				eflg++;
				break;
			case 'f':
				user_inactive = atoi (optarg);
				fflg++;
				break;
#endif	/* SHADOWPWD */
			case 'g':
				if (isdigit (optarg[0]))
					grp = getgrgid (atoi (optarg));
				else
					grp = getgrnam (optarg);

				if (! grp) {
					fprintf (stderr,
						"%s: unknown group %s\n",
						Prog, optarg);
					exit (1);
				}
				user_newgid = grp->gr_gid;
				gflg++;
				break;
			case 'G':
				Gflg++;
				if (get_groups (optarg))
					exit (1);

				break;
			case 'l':
				if (! VALID (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}

				/*
				 * If the name does not really change, we
				 * mustn't set the flag as this will cause
				 * rather serious problems later!
				 */

				if (strcmp (user_newname, optarg)) {
					strcpy (user_newname, optarg);
					lflg++;
				}
				break;
			case 'm':
				if (! dflg)
					usage ();

				mflg++;
				break;
			case 'o':
				if (! uflg)
					usage ();

				oflg++;
				break;
			case 's':
				if (! VALID (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				strncpy (user_shell, optarg, BUFSIZ);
				sflg++;
				break;
			case 'u':
				uflg++;
				user_newid = atoi (optarg);
				break;
			default:
				usage ();
		}
		anyflag++;
	}
	if (anyflag == 0) {
		fprintf (stderr, "%s: no flags given\n", Prog);
		exit (1);
	}
	if (optind != argc - 1)
		usage ();

	if (dflg && strcmp (user_home, user_newhome) == 0)
		dflg = mflg = 0;

	if (uflg && user_id == user_newid)
		uflg = oflg = 0;

	if (lflg && getpwnam (user_newname)) {
		fprintf (stderr, "%s: user %s exists\n", Prog, user_newname);
		exit (9);
	}
}

/*
 * close_files - close all of the files that were opened
 *
 *	close_files() closes all of the files that were opened for this
 *	new user.  This causes any modified entries to be written out.
 */

close_files ()
{
	if (! pw_close ()) {
		fprintf (stderr, "%s: cannot rewrite password file\n", Prog);
		fail_exit (1);
	}
#ifdef	SHADOWPWD
	if (! spw_close ()) {
		fprintf (stderr, "%s: cannot rewrite shadow password file\n",	
			Prog);
		fail_exit (1);
	}
#endif
	if (user_ngroups >= 0) {
		if (! gr_close ()) {
			fprintf (stderr, "%s: cannot rewrite group file\n",
				Prog);
			fail_exit (1);
		}
	}
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	if (user_ngroups >= 0) {
		if (! sgr_close ()) {
			fprintf (stderr, "%s: cannot rewrite shadow group file\n",
				Prog);
			fail_exit (1);
		}
	}
	(void) sgr_unlock ();
#endif
	(void) spw_unlock ();
	(void) pw_unlock ();

	/*
	 * Close the DBM and/or flat files
	 */

	endpwent ();
#ifdef	SHADOWPWD
	endspent ();
#endif
	endgrent ();
#ifdef	SHADOWGRP
	endsgent ();
#endif
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
		fprintf (stderr, "%s: cannot lock shadow password file\n",
			Prog);
		fail_exit (1);
	}
	if (! spw_open (O_RDWR)) {
		fprintf (stderr, "%s: cannot open shadow password file\n",
			Prog);
		fail_exit (1);
	}
#endif
}

/*
 * usr_update - create the user entries
 *
 *	usr_update() creates the password file entries for this user
 *	and will update the group entries if required.
 */

usr_update ()
{
	struct	passwd	pwent;
	struct	passwd	*pwd;
#ifdef	SHADOWPWD
	struct	spwd	spent;
	struct	spwd	*spwd;
#endif
	char	old_auth[BUFSIZ];
	char	auth_buf[BUFSIZ];

	old_auth[0] = '\0';

	/*
	 * Locate the entry in /etc/passwd, which MUST exist.
	 */

	pwd = pw_locate (user_name);
	pwent = *pwd;
	new_pwent (&pwent);

#ifdef	SHADOWPWD

	/* 
	 * Locate the entry in /etc/shadow.  It doesn't have to
	 * exist, and won't be created if it doesn't.
	 */

	if (spwd = spw_locate (user_name)) {
		spent = *spwd;
		new_spent (&spent);
		strcpy (old_auth, spent.sp_pwdp);
	} else {
		strcpy (old_auth, pwent.pw_passwd);
	}
#else
	strcpy (old_auth, pwent.pw_passwd);
#endif
	if (lflg || (Aflg && strcmp (old_auth, user_auth) != 0)) {
		convert_auth (user_auth, old_auth, auth_arg);
		delete_auths (old_auth, user_auth, auth_buf);
		if (auth_buf[0] && pw_auth (auth_buf, user_name,
				PW_DELETE, 0)) {
			fprintf (stderr,
				"%s: error deleting authentication method\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error deleting auth for `%s'\n",
				user_name);
#endif
			fail_exit (1);
		}
		add_auths (old_auth, user_auth, auth_buf);
		if (auth_buf[0] == '@' && pw_auth (auth_buf,
				lflg ? user_newname:user_name, PW_ADD, 0)) {
			fprintf (stderr,
				"%s: error adding authentication method\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error adding auth for `%s'\n",
				lflg ? user_newname:user_name);
#endif
			fail_exit (1);
		}
		update_auths (old_auth, user_auth, auth_buf);
		if (lflg && auth_buf[0] == '@' && pw_auth (auth_buf,
				user_newname, PW_CHANGE, user_name)) {
			fprintf (stderr,
				"%s: error changing authentication method\n",
				Prog);
#ifdef	USE_SYSLOG
			syslog (LOG_ERR, "error changing auth for `%s'\n",
				lflg ? user_newname:user_name);
#endif
			fail_exit (1);
		}
#ifdef	SHADOWPWD
		spent.sp_pwdp = user_auth;
#else
		pwent.pw_passwd = user_auth;
#endif
	}
	if (lflg || uflg || gflg || cflg || dflg || sflg || Aflg) {
		if (! pw_update (&pwent)) {
			fprintf (stderr, "%s: error changing password entry\n",
				Prog);
			fail_exit (1);
		}
		if (lflg && ! pw_remove (user_name)) {
			fprintf (stderr, "%s: error removing password entry\n",
				Prog);
			fail_exit (1);
		}
#if defined(DBM) || defined(NDBM)
		if (access ("/etc/passwd.pag", 0) == 0) {
			if (! pw_dbm_update (&pwent)) {
				fprintf (stderr,
					"%s: error adding password dbm entry\n",
					Prog);
				fail_exit (1);
			}
			if (lflg && (pwd = getpwnam (user_name)) &&
					! pw_dbm_remove (pwd)) {
				fprintf (stderr,
					"%s: error removing passwd dbm entry\n",
					Prog);
				fail_exit (1);
			}
		}
#endif
	}
#ifdef	SHADOWPWD
	if (spwd && (lflg || eflg || fflg || Aflg)) {
		if (! spw_update (&spent)) {
			fprintf (stderr,
				"%s: error adding new shadow password entry\n",
				Prog);
			fail_exit (1);
		}
		if (lflg && ! spw_remove (user_name)) {
			fprintf (stderr,
				"%s: error removing shadow password entry\n",
				Prog);
			fail_exit (1);
		}
	}
#ifdef	NDBM
	if (spwd && access ("/etc/shadow.pag", 0) == 0) {
		if (! sp_dbm_update (&spent)) {
			fprintf (stderr,
				"%s: error updating shadow passwd dbm entry\n",
				Prog);
			fail_exit (1);
		}
		if (lflg && ! sp_dbm_remove (user_name)) {
			fprintf (stderr,
				"%s: error removing shadow passwd db entry\n",
				Prog);
			fail_exit (1);
		}
	}
#endif	/* NDBM */
#endif	/* SHADOWPWD */
	if (Gflg || lflg)
		grp_update ();
}

/*
 * move_home - move the user's home directory
 *
 *	move_home() moves the user's home directory to a new location.
 *	The files will be copied if the directory cannot simply be
 *	renamed.
 */

move_home ()
{
	struct	stat	sb;

	if (mflg && stat (user_home, &sb) == 0) {
		if (access (user_newhome, 0) == 0) {
			fprintf (stderr, "%s: directory %s exists\n",
				Prog, user_newhome);
			fail_exit (12);
		} else if (rename (user_home, user_newhome)) {
			if (errno == EXDEV) {
				if (mkdir (user_newhome, sb.st_mode & 0777)) {
					fprintf (stderr,
						"%s: can't create %s\n",
						Prog, user_newhome);
				}
				if (chown (user_newhome,
						sb.st_uid, sb.st_gid)) {
					fprintf (stderr, "%s: can't chown %s\n",
						Prog, user_newhome);
					rmdir (user_newhome);
					fail_exit (12);
				}
#ifdef	DIR_ANY
				if (copy_tree (user_home, user_newhome,
						uflg ? user_newid:-1,
						gflg ? user_newgid:-1,
						user_id, user_gid) == 0 &&
					remove_tree (user_home) == 0 &&
						rmdir (user_home) == 0)
					return;

				(void) remove_tree (user_newhome);
				(void) rmdir (user_newhome);
#else
				return;
#endif
			}
			fprintf (stderr,
				"%s: cannot rename directory %s to %s\n",
				Prog, user_home, user_newhome);
			fail_exit (12);
		}
	}
	if (uflg || gflg)
		chown (dflg ? user_newhome:user_home,
			uflg ? user_newid:user_id,
			gflg ? user_newgid:user_gid);
}

/*
 * update_files - update the lastlog and faillog files
 */

void
update_files ()
{
	struct	lastlog	ll;
	struct	faillog	fl;
	int	fd;

	/*
	 * Relocate the "lastlog" entries for the user.  The old entry
	 * is left alone in case the UID was shared.  It doesn't hurt
	 * anything to just leave it be.
	 */

	if ((fd = open ("/usr/adm/lastlog", O_RDWR)) != -1) {
		lseek (fd, (long) user_id * sizeof ll, 0);
		if (read (fd, &ll, sizeof ll) == sizeof ll) {
			lseek (fd, (long) user_newid * sizeof ll, 0);
			write (fd, &ll, sizeof ll);
		}
		close (fd);
	}

	/*
	 * Relocate the "faillog" entries in the same manner.
	 */

	if ((fd = open (FAILFILE, O_RDWR)) != -1) {
		lseek (fd, (long) user_id * sizeof fl, 0);
		if (read (fd, &fl, sizeof fl) == sizeof fl) {
			lseek (fd, (long) user_newid * sizeof ll, 0);
			write (fd, &fl, sizeof fl);
		}
		close (fd);
	}
}

/*
 * fail_exit - exit with an error code after unlocking files
 */

fail_exit (code)
int	code;
{
	(void) gr_unlock ();
#ifdef	SHADOWGRP
	(void) sgr_unlock ();
#endif
#ifdef	SHADOWPWD
	(void) spw_unlock ();
#endif
	(void) pw_unlock ();
	exit (code);
}

/*
 * main - usermod command
 */

main (argc, argv)
int	argc;
char	**argv;
{
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
	 * The open routines for the NDBM files don't use read-write
	 * as the mode, so we have to clue them in.
	 */

#ifdef	NDBM
	pw_dbm_mode = O_RDWR;
#ifdef	SHADOWPWD
	sp_dbm_mode = O_RDWR;
#endif
	gr_dbm_mode = O_RDWR;
#ifdef	SHADOWGRP
	sg_dbm_mode = O_RDWR;
#endif
#endif	/* NDBM */
	process_flags (argc, argv);

	/*
	 * Do the hard stuff - open the files, change the user entries,
	 * change the home directory, then close and update the files.
	 */

	open_files ();

	usr_update ();

	close_files ();

	if (mflg)
		move_home ();

	if (uflg) {
		update_files ();

		/*
		 * Change the UID on all of the files owned by `user_id'
		 * to `user_newid' in the user's home directory.
		 */

		chown_tree (dflg ? user_newhome:user_home,
			user_id, user_newid,
			user_gid, gflg ? user_newgid:user_gid);
	}
	exit (0);
	/*NOTREACHED*/
}
