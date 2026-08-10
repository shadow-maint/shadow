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
static	char	sccsid[] = "@(#)useradd.c	3.14	08:20:53	23 Aug 1993";
#endif

#include "config.h"
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

GID_T	def_group;
char	def_gname[16];
char	def_home[BUFSIZ];
char	def_shell[BUFSIZ];
char	def_template[BUFSIZ] = "/etc/skel";
#ifdef	SHADOWPWD
long	def_inactive;
#endif
long	def_expire;
#ifdef	SVR4
char	def_file[] = "/usr/sadm/defadduser";
#else
char	def_file[] = "/etc/default/useradd";
#endif

#ifndef	NGROUPS_MAX
#define	NGROUPS_MAX	64
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
UID_T	user_id;
GID_T	user_gid;
char	user_comment[BUFSIZ];
char	user_home[BUFSIZ];
char	user_shell[BUFSIZ];
#ifdef	SHADOWPWD
long	user_expire;
#endif
int	user_ngroups;
GID_T	user_groups[NGROUPS_MAX];
char	user_auth[BUFSIZ];

char	*Prog;
char	*auth_arg;

int	uflg;	/* specify user ID for new account                            */
int	oflg;	/* permit non-unique user ID to be specified with -u          */
int	gflg;	/* primary group ID  for new account                          */
int	Gflg;	/* secondary group set for new account                        */
int	dflg;	/* home directory for new account                             */
int	bflg;	/* new default root of home directory                         */
int	sflg;	/* shell program for new account                              */
int	cflg;	/* comment (GECOS) field for new account                      */
int	mflg;	/* create user's home directory if it doesn't exist           */
int	kflg;	/* specify a directory to fill new user directory             */
int	fflg;	/* days until account with expired password is locked         */
int	eflg;	/* days after password changed before it becomes expired      */
int	Dflg;	/* set/show new user default values                           */
int	Aflg;	/* specify authentication method for user                     */

#ifdef NDBM
extern	int	pw_dbm_mode;
#ifdef	SHADOWPWD
extern	int	sp_dbm_mode;
#endif
extern	int	gr_dbm_mode;
#ifdef	SHADOWGRP
extern	int	sg_dbm_mode;
#endif
#endif

int	home_added;
int	pw_dbm_added;
#ifdef	NDBM
int	gr_dbm_added;
#ifdef	SHADOWPWD
int	sp_dbm_added;
#endif
#ifdef	SHADOWGRP
int	sg_dbm_added;
#endif
#endif	/* NDBM */

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
extern	int	pw_lock();
extern	int	pw_unlock();
extern	int	pw_rewind();
extern	int	pw_open();

#ifdef	SHADOWPWD
extern	int	spw_lock();
extern	int	spw_unlock();
extern	int	spw_open();
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
 * get_defaults - read the defaults file
 *
 *	get_defaults() reads the defaults file for this command.  It sets
 *	the various values from the file, or uses built-in default values
 *	if the file does not exist.
 */

void
get_defaults ()
{
	FILE	*fp;
	char	buf[BUFSIZ];
	char	*cp;
	char	*cp2;
	struct	group	*grp;

	/*
	 * Open the defaults file for reading.
	 */

	if (! (fp = fopen (def_file, "r"))) {

		/*
		 * No defaults file - set up the defaults that are given
		 * in the documentation.
		 */

		def_group = 1;
		strcpy (def_gname, "other");
		strcpy (def_home, "/home");
#ifdef	SHADOWPWD
		def_inactive = 0;
#endif
		def_expire = 0;
		return;
	}

	/*
	 * Read the file a line at a time.  Only the lines that have
	 * relevant values are used, everything else can be ignored.
	 */

	while (fgets (buf, BUFSIZ, fp)) {
		if (cp = strrchr (buf, '\n'))
			*cp = '\0';

		if (! (cp = strchr (buf, '=')))
			continue;

		cp++;

		/*
		 * Primary GROUP identifier
		 */

#ifdef	SVR4
		if (strncmp ("defgroup=", buf, 9) == 0)
#else
		if (strncmp ("GROUP=", buf, 6) == 0)
#endif
		{
			if (isdigit (*cp)) {
				def_group = atoi (cp);
				if (! (grp = getgrgid (def_group))) {
					fprintf (stderr, "%s: unknown gid %s\n",
						Prog, cp);
				}
				strcpy (def_gname, grp->gr_name);
			} else if (grp = getgrnam (cp)) {
				def_group = grp->gr_gid;
				strncpy (def_gname, cp, sizeof def_gname);
				def_gname[sizeof def_gname - 1] = 0;
			} else {
				fprintf (stderr, "%s: unknown group %s\n",
					Prog, cp);
			}
		}
		
		/*
		 * Default HOME filesystem
		 */
		 
#ifdef	SVR4
		else if (strncmp ("defparent=", buf, 10) == 0)
#else
		else if (strncmp ("HOME=", buf, 5) == 0)
#endif
		{
			strncpy (def_home, cp, BUFSIZ);
		}

		/*
		 * Default Login Shell command
		 */

#ifdef	SVR4
		else if (strncmp ("defshell=", buf, 9) == 0)
#else
		else if (strncmp ("SHELL=", buf, 6) == 0)
#endif
		{
			strncpy (def_shell, cp, BUFSIZ);
		}

#ifdef	SHADOWPWD
		/*
		 * Default Password Inactive value
		 */

#ifdef	SVR4
		else if (strncmp ("definact=", buf, 9) == 0)
#else
		else if (strncmp ("INACTIVE=", buf, 9) == 0)
#endif
		{
			def_inactive = atoi (cp);
		}
#endif
		
		/*
		 * Default Password Expiration value
		 */

#ifdef	SVR4
		else if (strncmp ("defexpire=", buf, 10) == 0)
#else
		else if (strncmp ("EXPIRE=", buf, 7) == 0)
#endif
		{
			if (*cp == '\0')
				def_expire = -1;
			else
				def_expire = atoi (cp);
		}
	}
}

/*
 * show_defaults - show the contents of the defaults file
 *
 *	show_defaults() displays the values that are used from the default
 *	file and the built-in values.
 */

void
show_defaults ()
{
#ifdef	SVR4
	struct	tm	*tm;
	time_t	time;

	time = def_expire * (3600L*24L);
	tm = gmtime (&time);

	printf ("group=%s,%d  basedir=%s  skel=%s\n",
		def_gname, def_group, def_home, def_template);

	printf ("shell=%s  inactive=%d  ", def_shell, def_inactive);

	if (def_expire >= 0)
		printf ("expire=%d/%d/%d\n",
			tm->tm_mon + 1, tm->tm_mday,
			tm->tm_year >= 100 ? tm->tm_year + 1900:tm->tm_year);
	else
		printf ("expire=\n");
#else
	printf ("GROUP=%d\n", def_group);
	printf ("HOME=%s\n", def_home);
#ifdef	SHADOWPWD
	printf ("INACTIVE=%d\n", def_inactive);
#endif
	printf ("EXPIRE=%d\n", def_expire);
	printf ("SHELL=%s\n", def_shell);
	printf ("SKEL=%s\n", def_template);
#endif

}

/*
 * set_defaults - write new defaults file
 *
 *	set_defaults() re-writes the defaults file using the values that
 *	are currently set.  Duplicated lines are pruned, missing lines are
 *	added, and unrecognized lines are copied as is.
 */

int
set_defaults ()
{
	FILE	*ifp;
	FILE	*ofp;
	char	buf[BUFSIZ];
#ifdef	SVR4
	static	char	new_file[] = "/usr/sadm/defuXXXXXX";
#else
	static	char	new_file[] = "/etc/default/nuaddXXXXXX";
#endif
	char	*cp;
	int	out_group = 0;
	int	out_home = 0;
	int	out_inactive = 0;
	int	out_expire = 0;
	int	out_shell = 0;
	int	out_skel = 0;
#ifdef	SVR4
	int	out_gname = 0;
#endif

	/*
	 * Create a temporary file to copy the new output to.
	 */

	mktemp (new_file);
	if (! (ofp = fopen (new_file, "w"))) {
		fprintf (stderr, "%s: cannot create new defaults file\n", Prog);
		return -1;
	}

	/*
	 * Open the existing defaults file and copy the lines to the
	 * temporary files, using any new values.  Each line is checked
	 * to insure that it is not output more than once.
	 */

	if (ifp = fopen (def_file, "r")) {
		while (fgets (buf, BUFSIZ, ifp)) {
			if (cp = strrchr (buf, '\n'))
				*cp = '\0';

#ifdef	SVR4
			if (strncmp ("defgroup=", buf, 9) == 0)
#else
			if (strncmp ("GROUP=", buf, 6) == 0)
#endif
			{
				if (! out_group)
#ifdef	SVR4
					fprintf (ofp, "defgroup=%d\n", def_group);
#else
					fprintf (ofp, "GROUP=%d\n", def_group);
#endif
				out_group++;
			}
#ifdef	SVR4
			else if (strncmp ("defgname=", buf, 9) == 0)
			{
				if (! out_gname)
					fprintf (ofp, "defgname=%s\n", def_gname);
				out_gname++;
			}
#endif
#ifdef	SVR4
			else if (strncmp ("defparent=", buf, 10) == 0)
#else
			else if (strncmp ("HOME=", buf, 5) == 0)
#endif
			{
				if (! out_home)
#ifdef	SVR4
					fprintf (ofp, "defparent=%s\n", def_home);
#else
					fprintf (ofp, "HOME=%s\n", def_home);
#endif
				out_home++;
#ifdef	SHADOWPWD
			}
#ifdef	SVR4
			else if (strncmp ("definact=", buf, 9) == 0)
#else
			else if (strncmp ("INACTIVE=", buf, 9) == 0)
#endif
			{
				if (! out_inactive)
#ifdef	SVR4
					fprintf (ofp, "definact=%d\n",
						def_inactive);
#else
					fprintf (ofp, "INACTIVE=%d\n",
						def_inactive);
#endif
				out_inactive++;
#endif
			}
#ifdef	SVR4
			else if (strncmp ("defexpire=", buf, 10) == 0)
#else
			else if (strncmp ("EXPIRE=", buf, 7) == 0)
#endif
			{
				if (! out_expire)
#ifdef	SVR4
					if (def_expire >= 0)
						fprintf (ofp, "defexpire=%d\n",
							def_expire);
					else
						fprintf (ofp, "defexpire=\n");
#else
					fprintf (ofp, "EXPIRE=%d\n",
						def_expire);
#endif
				out_expire++;
			}
#ifdef	SVR4
			else if (strncmp ("defshell=", buf, 9) == 0)
#else
			else if (strncmp ("SHELL=", buf, 6) == 0)
#endif
			{
				if (! out_shell)
#ifdef	SVR4
					fprintf (ofp, "defshell=%s\n",
						def_shell);
#else
					fprintf (ofp, "SHELL=%s\n",
						def_shell);
#endif
				out_shell++;
			}
#ifdef	SVR4
			else if (strncmp ("defskel=", buf, 8) == 0)
#else
			else if (strncmp ("SKEL=", buf, 5) == 0)
#endif
			{
				if (! out_skel)
#ifdef	SVR4
					fprintf (ofp, "defskel=%s\n",
						def_template);
#else
					fprintf (ofp, "SKEL=%s\n",
						def_template);
#endif
				out_skel++;
			}
			else
				fprintf (ofp, "%s\n", buf);
		}
		fclose ((FILE *) ifp);
	}

	/*
	 * Check each line to insure that every line was output.  This
	 * causes new values to be added to a file which did not previously
	 * have an entry for that value.
	 */

	if (! out_group)
#ifdef	SVR4
		fprintf (ofp, "defgroup=%d\n", def_group);
#else
		fprintf (ofp, "GROUP=%d\n", def_group);
#endif
	if (! out_home)
#ifdef	SVR4
		fprintf (ofp, "defparent=%s\n", def_home);
#else
		fprintf (ofp, "HOME=%s\n", def_home);
#endif
#ifdef	SHADOWPWD
	if (! out_inactive)
#ifdef	SVR4
		fprintf (ofp, "definact=%d\n", def_inactive);
#else
		fprintf (ofp, "INACTIVE=%d\n", def_inactive);
#endif
#endif
	if (! out_expire)
#ifdef	SVR4
		fprintf (ofp, "defexpire=%d\n", def_expire);
#else
		fprintf (ofp, "EXPIRE=%d\n", def_expire);
#endif
	if (! out_shell)
#ifdef	SVR4
		fprintf (ofp, "defshell=%s\n", def_shell);
#else
		fprintf (ofp, "SHELL=%s\n", def_shell);
#endif
	if (! out_skel)
#ifdef	SVR4
		fprintf (ofp, "defskel=%s\n", def_template);
#else
		fprintf (ofp, "SKEL=%s\n", def_template);
#endif

	/*
	 * Flush and close the file.  Check for errors to make certain
	 * the new file is intact.
	 */

	(void) fflush (ofp);
	if (ferror (ofp) || fclose ((FILE *) ofp)) {
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the current default file to its backup name.
	 */

	sprintf (buf, "%s-", def_file);
	if (rename (def_file, buf) && errno != ENOENT) {
		sprintf (buf, "%s: rename: %s", Prog, def_file);
		perror (buf);
		unlink (new_file);
		return -1;
	}

	/*
	 * Rename the new default file to its correct name.
	 */

	if (rename (new_file, def_file)) {
		sprintf (buf, "%s: rename: %s", Prog, new_file);
		perror (buf);
		return -1;
	}
#ifdef	USE_SYSLOG
#ifdef	SHADOWPWD
	syslog (LOG_INFO,
		"defaults: group=%d, home=%s, inactive=%d, expire=%d\n",
		def_group, def_home, def_inactive, def_expire);
#else
	syslog (LOG_INFO,
		"defaults: group=%d, home=%s, expire=%d\n",
		def_group, def_home, def_expire);
#endif
#endif
	return 0;
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
		list = cp;

#ifdef	USE_NIS
		/*
		 * Don't add this group if they are an NIS group.  Tell
		 * the user to go to the server for this group.
		 */

		if (__isgrNIS ()) {
			fprintf (stderr, "%s: group `%s' is a NIS group.\n",
				Prog, grp->gr_name);
			continue;
		}
#endif
		/*
		 * Add the GID value from the group file to the user's
		 * list of groups.
		 */

		user_groups[user_ngroups++] = grp->gr_gid;
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
		"usage:\t%s [-u uid [-o]] [-g group] [-G group,...] \n", Prog);
	fprintf (stderr,
		"\t\t[-d home] [-s shell] [-c comment] [-m [-k template]]\n");
#ifdef	MDY_DATE
	fprintf (stderr,
#ifdef	SHADOWPWD
		"\t\t[-f inactive ] [-e expire mm/dd/yy ] [ -A program ] name\n"
#else
		"\t\t[ -A program ] name\n"
#endif
		);
#endif
#ifdef	DMY_DATE
	fprintf (stderr,
#ifdef	SHADOWPWD
		"\t\t[-f inactive ] [-e expire dd/mm/yy ] [ -A program ] name\n"
#else
		"\t\t[ -A program ] name\n"
#endif
		);
#endif
#ifdef	YMD_DATE
	fprintf (stderr,
#ifdef	SHADOWPWD
		"\t\t[-f inactive ] [-e expire yy/mm/dd ] [ -A program ] name\n"
#else
		"\t\t[ -A program ] name\n"
#endif
		);
#endif
	fprintf (stderr,
#ifdef	SHADOWPWD
		"\t%s -D [-g group] [-b base] [-f inactive] [-e expire]\n",
#else
		"\t%s -D [-g group] [-b base] [-e expire]\n",
#endif
			Prog);

	exit (1);
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
#if !defined(SHADOWPWD) && defined(ATT_AGE)
	static	char	age[3];
#endif

	memset (pwent, 0, sizeof *pwent);
	pwent->pw_name = user_name;
#ifdef	SHADOWPWD
	pwent->pw_passwd = "*";
#ifdef	ATT_AGE
	pwent->pw_age = "";
#endif	/* ATT_AGE */
#else	/* !SHADOWPWD */
	if (Aflg)
		pwent->pw_passwd = user_auth;
	else
		pwent->pw_passwd = "!";

#ifdef	ATT_AGE
	pwent->pw_age = age;
	age[0] = i64c (def_expire + 6 / 7);
	age[1] = i64c (0);
	age[2] = '\0';
#endif
#endif
	pwent->pw_uid = user_id;
	pwent->pw_gid = user_gid;
	pwent->pw_gecos = user_comment;
#ifdef	ATT_COMMENT
	pwent->pw_comment = "";
#endif
	pwent->pw_dir = user_home;
	pwent->pw_shell = user_shell;
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
	memset (spent, 0, sizeof *spent);
	spent->sp_namp = user_name;

	if (Aflg)
		spent->sp_pwdp = user_auth;
	else
		spent->sp_pwdp = "!";

	spent->sp_lstchg = 0;
	spent->sp_min = 0;
	spent->sp_max = def_expire;
	spent->sp_warn = 0;
	spent->sp_inact = def_inactive;
	spent->sp_expire = user_expire;
}
#endif

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
	struct	group	*grp;
#ifdef	SHADOWGRP
	struct	sgrp	*sgrp;
#endif

	/*
	 * Lock and open the group file.  This will load all of the group
	 * entries.
	 */

	if (! gr_lock ()) {
		fprintf (stderr, "%s: error locking group file\n", Prog);
		exit (1);
	}
	if (! gr_open (O_RDWR)) {
		fprintf (stderr, "%s: error opening group file\n", Prog);
		exit (1);
	}
#ifdef	SHADOWGRP
	if (! sgr_lock ()) {
		fprintf (stderr, "%s: error locking shadow group file\n", Prog);
		exit (1);
	}
	if (! sgr_open (O_RDWR)) {
		fprintf (stderr, "%s: error opening shadow group file\n", Prog);
		exit (1);
	}
#endif

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

		if (i == user_ngroups)
			continue;

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */

		grp->gr_mem = add_list (grp->gr_mem, user_name);
		if (! gr_update (grp)) {
			fprintf (stderr, "%s: error adding new group entry\n",
				Prog);
			fail (1);
		}
#ifdef	NDBM
		/*
		 * Update the DBM group file with the new entry as well.
		 */

		if (! gr_dbm_update (grp)) {
			fprintf (stderr, "%s: cannot add new dbm group entry\n",
				Prog);
			fail (1);
		} else
			gr_dbm_added++;
#endif
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "add `%s' to group `%s'\n",
			user_name, grp->gr_name);
#endif
	}
#ifdef NDBM
	endgrent ();
#endif

#ifdef	SHADOWGRP
	/*
	 * Scan through the entire shadow group file looking for the groups
	 * that the user is a member of.  The administrative list isn't
	 * modified.
	 */

	for (sgr_rewind (), sgrp = sgr_next ();sgrp;sgrp = sgr_next ()) {

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
		if (i == user_ngroups)
			continue;

		/* 
		 * Add the username to the list of group members and
		 * update the group entry to reflect the change.
		 */

		sgrp->sg_mem = add_list (sgrp->sg_mem, user_name);
		if (! sgr_update (sgrp)) {
			fprintf (stderr, "%s: error adding new group entry\n",
				Prog);
			fail (1);
		}
#ifdef	NDBM
		/*
		 * Update the DBM group file with the new entry as well.
		 */

		if (! sg_dbm_update (sgrp)) {
			fprintf (stderr, "%s: cannot add new dbm group entry\n",
				Prog);
			fail (1);
		} else
			sg_dbm_added++;
#endif	/* NDBM */
#ifdef	USE_SYSLOG
		syslog (LOG_INFO, "add `%s' to shadow group `%s'\n",
			user_name, sgrp->sg_name);
#endif	/* USE_SYSLOG */
	}
#ifdef NDBM
	endsgent ();
#endif	/* NDBM */
#endif	/* SHADOWGRP */
}

/*
 * find_new_uid - find the next available UID
 *
 *	find_new_uid() locates the next highest unused UID in the password
 *	file, or checks the given user ID against the existing ones for
 *	uniqueness.
 */

int
find_new_uid ()
{
	struct	passwd	*pwd;

	/*
	 * Start with some UID value if the user didn't provide us with
	 * one already.
	 */

	if (! uflg)
		user_id = 100;

	/*
	 * Search the entire password file, either looking for this
	 * UID (if the user specified one with -u) or looking for the
	 * largest unused value.
	 */

	for (pw_rewind (), pwd = pw_next ();pwd;pwd = pw_next ()) {
		if (strcmp (user_name, pwd->pw_name) == 0) {
			fprintf (stderr, "%s: name %s is not unique\n",
				Prog, user_name);
			exit (1);
		}
		if (uflg && user_id == pwd->pw_uid) {
			fprintf (stderr, "%s: uid %d is not unique\n",
				Prog, user_id);
			exit (1);
		}
		if (! uflg && pwd->pw_uid >= user_id)
			user_id = pwd->pw_uid + 1;
	}
}

/*
 * convert_auth - convert the argument list to a authentication list
 */

convert_auth (auths, list)
char	*auths;
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
	int	anyflag = 0;
	int	arg;

	while ((arg = getopt (argc, argv,
#ifdef	SHADOWPWD
		"A:Du:og:G:d:s:c:mk:f:e:b:"
#else
		"A:Du:og:G:d:s:c:mk:e:b:"
#endif
					)) != EOF) {
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
			case 'b':
				if (! VALID (optarg) || optarg[0] != '/') {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				bflg++;
				if (! Dflg)
					usage ();

				strncpy (def_home, optarg, BUFSIZ);
				break;
			case 'c':
				if (! VALID (optarg)) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				cflg++;
				strncpy (user_comment, optarg, BUFSIZ);
				break;
			case 'd':
				if (! VALID (optarg) || optarg[0] != '/') {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				dflg++;
				strncpy (user_home, optarg, BUFSIZ);
				break;
			case 'D':
				if (anyflag)
					usage ();

				Dflg++;
				break;
			case 'e':
				eflg++;
				if (Dflg)
					def_expire = strtoday (optarg);
				else {
#ifdef	SHADOWPWD
					user_expire = atoi (optarg);
#ifdef	ITI_AGING
					user_expire *= DAY;
#endif
#else
					usage ();
#endif
				}
				break;
#ifdef	SHADOWPWD
			case 'f':
				fflg++;
				def_inactive = atoi (optarg);
				break;
#endif
			case 'g':
				gflg++;
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
				if (Dflg)
					def_group = grp->gr_gid;
				else
					user_gid = grp->gr_gid;
				break;
			case 'G':
				Gflg++;
				if (get_groups (optarg))
					exit (1);

				break;
			case 'k':
				if (! mflg)
					usage ();

				strncpy (def_template, optarg, BUFSIZ);
				kflg++;
				break;
			case 'm':
				mflg++;
				break;
			case 'o':
				if (! uflg)
					usage ();

				oflg++;
				break;
			case 's':
				if (! VALID (optarg) || (optarg[0] &&
						optarg[0] != '/')) {
					fprintf (stderr,
						"%s: invalid field `%s'\n",
						Prog, optarg);
					exit (3);
				}
				sflg++;
				strncpy (user_shell, optarg, BUFSIZ);
				strncpy (def_shell, user_shell, BUFSIZ);
				break;
			case 'u':
				uflg++;
				user_id = atoi (optarg);
				break;
			default:
				usage ();
		}
		anyflag++;
	}

	/*
	 * Get the user name if there is one.
	 */

	if (! Dflg && optind < argc) {
		if (! VALID (argv[optind]) || ! argv[optind][0] ||
				argv[optind][0] == '-' ||
				argv[optind][0] == '+') {
			fprintf (stderr,
				"%s: invalid user name `%s'\n",
				Prog, argv[optind]);
			exit (3);
		}
		strcpy (user_name, argv[optind]);
	}
	if (! dflg)
		sprintf (user_home, "%s/%s", def_home, user_name);

	if (! gflg)
		user_gid = def_group;

	if (Dflg) {
		if (optind != argc)
			usage ();

		if (uflg || oflg || Gflg || dflg ||
				cflg || mflg)
			usage ();
	} else {
		if (optind != argc - 1)
			usage ();
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
		fail (1);
	}
#ifdef	SHADOWPWD
	if (! spw_close ()) {
		fprintf (stderr, "%s: cannot rewrite shadow password file\n",	
			Prog);
		fail (1);
	}
#endif
	if (user_ngroups > 0) {
		if (! gr_close ()) {
			fprintf (stderr, "%s: cannot rewrite group file\n",
				Prog);
			fail (1);
		}
		(void) gr_unlock ();
#ifdef	SHADOWGRP
		if (! sgr_close ()) {
			fprintf (stderr, "%s: cannot rewrite shadow group file\n",
				Prog);
			fail (1);
		}
		(void) sgr_unlock ();
#endif
	}
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
		exit (1);
	}
#ifdef	SHADOWPWD
	if (! spw_lock ()) {
		fprintf (stderr, "%s: cannot lock shadow password file\n", Prog);
		exit (1);
	}
	if (! spw_open (O_RDWR)) {
		fprintf (stderr, "%s: cannot open shadow password file\n", Prog);
		exit (1);
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
#ifdef	SHADOWPWD
	struct	spwd	spent;
#endif
#ifdef	USE_SYSLOG
	char	buf[BUFSIZ];
#endif

	if (! oflg)
		find_new_uid ();

	if (Aflg)
		convert_auth (user_auth, auth_arg);

	/*
	 * Fill in the password structure with any new fields, making
	 * copies of strings.
	 */

	new_pwent (&pwent);
#ifdef	SHADOWPWD
	new_spent (&spent);
#endif
#ifdef	USE_SYSLOG

	/*
	 * Create a syslog entry.  We need to do this now in case anything
	 * happens so we know what we were trying to accomplish.
	 */

	sprintf (buf,
		"new user: name=%s, uid=%d, gid=%d, home=%s, shell=%s, auth=%s\n",
		user_name, user_id, user_gid, user_home, user_shell,
			Aflg ? auth_arg:"DEFAULT");
	syslog (LOG_INFO, buf);
#endif

	/*
	 * Attempt to add the new user to any authentication programs
	 * which have been requested.  Since this is more likely to fail
	 * than the update of the password file, we do this first.
	 */

	if (Aflg && pw_auth (user_auth, pwent.pw_name, PW_ADD, 0)) {
		fprintf (stderr, "%s: error adding authentication method\n",
			Prog);
		fail (1);
	}

	/*
	 * Put the new (struct passwd) in the table.
	 */

	if (! pw_update (&pwent)) {
		fprintf (stderr, "%s: error adding new password entry\n", Prog);
		exit (1);
	}
#if defined(DBM) || defined(NDBM)

	/*
	 * Update the DBM files.  This creates the user before the flat
	 * files are updated.  This is safe before the password field is
	 * either locked, or set to a valid authentication string.
	 */

	if (access ("/etc/passwd.pag", 0) == 0) {
		if (! pw_dbm_update (&pwent)) {
			fprintf (stderr,
				"%s: error updating password dbm entry\n",
				Prog);
			exit (1);
		} else
			pw_dbm_added = 1;
	}
	endpwent ();
#endif
#ifdef	SHADOWPWD

	/*
	 * Put the new (struct spwd) in the table.
	 */

	if (! spw_update (&spent)) {
		fprintf (stderr, "%s: error adding new shadow password entry\n",
			Prog);
		exit (1);
	}
#ifdef	NDBM

	/* 
	 * Update the DBM files for the shadow password.  This entry is
	 * output before the entry in the flat file, but this is safe as
	 * the password is locked or the authentication string has the
	 * proper values.
	 */

	if (access ("/etc/shadow.pag", 0) == 0) {
		if (! sp_dbm_update (&spent)) {
			fprintf (stderr,
				"%s: error updating shadow passwd dbm entry\n",
				Prog);
			fail (1);
		} else
			sp_dbm_added++;
	}
	endspent ();
#endif
#endif	/* SHADOWPWD */

	/*
	 * Do any group file updates for this user.
	 */

	if (user_ngroups > 0)
		grp_update ();
}

/*
 * create_home - create the user's home directory
 *
 *	create_home() creates the user's home directory if it does not
 *	already exist.  It will be created mode 755 owned by the user
 *	with the user's default group.
 */

create_home ()
{
	if (access (user_home, 0)) {
		if (mkdir (user_home, 0)) {
			fprintf (stderr, "%s: cannot create directory %s\n",
				Prog, user_home);
			fail (1);
		}
		chown (user_home, user_id, user_gid);
		chmod (user_home, 0755);
		home_added++;
	}
}

/*
 * fail - undo as much as possible
 */

fail (code)
int	code;
{
	struct	passwd	pwent;

#if defined(DBM) || defined(NDBM)
	if (pw_dbm_added) {
		pwent.pw_name = user_name;
		pwent.pw_uid = user_id;
		(void) pw_dbm_remove (&pwent);
	}
#endif
#ifdef	NDBM
	if (gr_dbm_added)
		fprintf (stderr, "%s: rebuild the group database\n", Prog);
#ifdef	SHADOWPWD
	if (sp_dbm_added)
		(void) sp_dbm_remove (user_name);
#endif
#ifdef	SHADOWGRP
	if (sg_dbm_added)
		fprintf (stderr, "%s: rebuild the shadow group database\n",
			Prog);
#endif
#endif	/* NDBM */
	if (home_added)
		rmdir (user_home);

#ifdef	USE_SYSLOG
	syslog (LOG_INFO, "failed adding user `%s', data deleted\n", user_name);
#endif
	exit (code);
}

/*
 * main - useradd command
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
#endif
	get_defaults ();

	process_flags (argc, argv);

	/*
	 * See if we are messing with the defaults file, or creating
	 * a new user.
	 */

	if (Dflg) {
		if (gflg || bflg || fflg || eflg)
			exit (set_defaults () ? 1:0);

		show_defaults ();
		exit (0);
	}

	/*
	 * Start with a quick check to see if the user exists.
	 */

	if (getpwnam (user_name)) {
		fprintf (stderr, "%s: user %s exists\n", Prog, user_name);
		exit (1);
	}

	/*
	 * Do the hard stuff - open the files, create the user entries,
	 * create the home directory, then close and update the files.
	 */

	open_files ();

	usr_update ();

	if (mflg) {
		create_home ();
#if defined(DIR_XENIX) || defined(DIR_BSD) || defined(DIR_SYSV)
		copy_tree (def_template, user_home, user_id, user_gid, -1, -1);
#endif
	}
	close_files ();
	exit (0);
	/*NOTREACHED*/
}
