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

#include "config.h"
#include <stdio.h>
#include "pwd.h"
#ifdef	BSD
#include <strings.h>
#else
#include <string.h>
#endif

/*
 * If AUTOSHADOW is enable, the getpwnam and getpwuid calls will
 * fill in the pw_passwd and pw_age fields from the passwd and
 * shadow files.
 */

#if defined(AUTOSHADOW) && !defined(SHADOWPWD)
#undef	AUTOSHADOW
#endif
#ifdef	AUTOSHADOW
#include "shadow.h"
#endif

/*
 * If DBM or NDBM is enabled, the getpwnam and getpwuid calls will
 * go to the database files to look for the requested entries.
 */

#ifdef	DBM
#include <dbm.h>
#endif
#ifdef	NDBM
#include <ndbm.h>
#include <fcntl.h>
DBM	*pw_dbm;
int	pw_dbm_mode = -1;
#endif

/*
 * ITI-style aging uses time_t's as the time fields, while
 * AT&T-style aging uses long numbers of days.
 */

#ifdef	ITI_AGING
#define	WEEK	(7L*24L*3600L)
#else
#define	WEEK	7
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)pwent.c	3.10	12:10:58	02 May 1993";
#endif

#define	SBUFSIZ	64
#define	NFIELDS	7

static	FILE	*pwdfp;
static	char	pwdbuf[BUFSIZ];
static	char	*pwdfile = PWDFILE;
#if defined(DBM) || defined(NDBM)
static	int	dbmopened;
static	int	dbmerror;
#endif
static	char	*pwdfields[NFIELDS];
static	struct	passwd	pwent;

#if defined(AUTOSHADOW) && defined(ATT_AGE) && defined(GETPWENT)
/*
 * sptopwage - convert shadow ages to AT&T-style pw_age ages
 *
 *	sptopwage() converts the values in the shadow password
 *	entry to the format used in the old-style password
 *	entry.
 */

static char *
sptopwage (spwd)
struct	spwd	*spwd;
{
	static	char	age[5];
	long	min;
	long	max;
	long	last;

	if ((min = (spwd->sp_min / WEEK)) < 0)
		min = 0;
	else if (min >= 64)
		min = 63;

	if ((max = (spwd->sp_max / WEEK)) < 0)
		max = 0;
	else if (max >= 64)
		max = 63;

	if ((last = (spwd->sp_lstchg / WEEK)) < 0)
		last = 0;
	else if (last >= 4096)
		last = 4095;

	age[0] = i64c (max);
	age[1] = i64c (min);
	age[2] = i64c (last % 64);
	age[3] = i64c (last / 64);
	age[4] = '\0';
	return age;
}
#endif

/*
 * sgetpwent - convert a string to a (struct passwd)
 *
 * sgetpwent() parses a string into the parts required for a password
 * structure.  Strict checking is made for the UID and GID fields and
 * presence of the correct number of colons.  Any failing tests result
 * in a NULL pointer being returned.
 *
 * NOTE: This function uses hard-coded string scanning functions for
 *	performance reasons.  I am going to come up with some conditional
 *	compilation glarp to improve on this in the future.
 */

struct passwd *
sgetpwent (buf)
char	*buf;
{
	register int	i;
	register char	*cp;
	char	*ep;

	/*
	 * Copy the string to a static buffer so the pointers into
	 * the password structure remain valid.
	 */

	strncpy (pwdbuf, buf, BUFSIZ);
	pwdbuf[BUFSIZ-1] = '\0';

	/*
	 * Save a pointer to the start of each colon separated
	 * field.  The fields are converted into NUL terminated strings.
	 */

	for (cp = pwdbuf, i = 0;i < NFIELDS && cp;i++) {
		pwdfields[i] = cp;
		while (*cp && *cp != ':')
			++cp;
	
		if (*cp)
			*cp++ = '\0';
		else
			cp = 0;
	}

	/*
	 * There must be exactly NFIELDS colon separated fields or
	 * the entry is invalid.  Also, the UID and GID must be non-blank.
	 */

	if (i != NFIELDS || *pwdfields[2] == '\0' || *pwdfields[3] == '\0')
		return 0;

	/*
	 * Each of the fields is converted the appropriate data type
	 * and the result assigned to the password structure.  If the
	 * UID or GID does not convert to an integer value, a NULL
	 * pointer is returned.
	 */

	pwent.pw_name = pwdfields[0];
	pwent.pw_passwd = pwdfields[1];
	if ((pwent.pw_uid = strtol (pwdfields[2], &ep, 10)) == 0 && *ep)
		return 0;

	if ((pwent.pw_gid = strtol (pwdfields[3], &ep, 10)) == 0 && *ep)
		return 0;
#ifdef	ATT_AGE
	cp = pwent.pw_passwd;
	while (*cp && *cp != ',')
		++cp;

	if (*cp) {
		*cp++ = '\0';
		pwent.pw_age = cp;
	} else {
		cp = 0;
		pwent.pw_age = "";
	}
#endif
	pwent.pw_gecos = pwdfields[4];
#ifdef	ATT_COMMENT
	pwent.pw_comment = "";
#endif
	pwent.pw_dir = pwdfields[5];
	pwent.pw_shell = pwdfields[6];

	return (&pwent);
}

#ifdef	GETPWENT

/*
 * fgetpwent - get a password file entry from a stream
 *
 * fgetpwent() reads the next line from a password file formatted stream
 * and returns a pointer to the password structure for that line.
 */

struct passwd *
fgetpwent (fp)
FILE	*fp;
{
	char	buf[BUFSIZ];

	while (fgets (buf, BUFSIZ, fp) != (char *) 0) {
		buf[strlen (buf) - 1] = '\0';
		return (sgetpwent (buf));
	}
	return 0;
}

/*
 * endpwent - close a password file
 *
 * endpwent() closes the password file if open.  if autoshadowing is
 * enabled the system must also end access to the shadow files since
 * the user is probably unaware it was ever accessed.
 */

int
endpwent ()
{
	if (pwdfp)
		if (fclose (pwdfp))
			return -1;

	pwdfp = 0;
#ifdef	NDBM
	if (dbmopened && pw_dbm) {
		dbm_close (pw_dbm);
		dbmopened = 0;
		dbmerror = 0;
		pw_dbm = 0;
	}
#endif
#ifdef	AUTOSHADOW
	endspent ();
#endif
	return 0;
}

/*
 * getpwent - get a password entry from the password file
 *
 * getpwent() opens the password file, if not already opened, and reads
 * a single entry.  NULL is returned if any errors are encountered reading
 * the password file.
 */

struct passwd *
getpwent ()
{
	if (! pwdfp && setpwent ())
		return 0;

	return fgetpwent (pwdfp);
}

/*
 * getpwuid - locate the password entry for a given UID
 *
 * getpwuid() locates the first password file entry for the given UID.
 * If there is a valid DBM file, the DBM files are queried first for
 * the entry.  Otherwise, a linear search is begun of the password file
 * searching for an entry which matches the provided UID.
 */

struct passwd *
getpwuid (uid)
uid_t	uid;
{
	struct	passwd	*pwd;
#if defined(DBM) || defined(NDBM)
	datum	key;
	datum	content;
#endif
#ifdef	AUTOSHADOW
	struct	spwd	*spwd;
#endif

	if (setpwent ())
		return 0;

#if defined(DBM) || defined(NDBM)

	/*
	 * If the DBM file are now open, create a key for this UID and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		pwent.pw_uid = uid;
		key.dsize = sizeof pwent.pw_uid;
		key.dptr = (char *) &pwent.pw_uid;
#ifdef	DBM
		content = fetch (key);
#endif
#ifdef	NDBM
		content = dbm_fetch (pw_dbm, key);
#endif
		if (content.dptr != 0) {
			memcpy (pwdbuf, content.dptr, content.dsize);
			pw_unpack (pwdbuf, content.dsize, &pwent);
#ifdef	AUTOSHADOW
			if (spwd = getspnam (pwent.pw_name)) {
				pwent.pw_passwd = spwd->sp_pwdp;
#ifdef	ATT_AGE
				pwent.pw_age = sptopwage (spwd);
#endif
			}
#endif
			return &pwent;
		}
	}
#endif
	/*
	 * Search for an entry which matches the UID.  Return the
	 * entry when a match is found.
	 */

	while (pwd = getpwent ())
		if (pwd->pw_uid == uid)
			break;

#ifdef	AUTOSHADOW
	if (pwd && (spwd = getspnam (pwd->pw_name))) {
		pwd->pw_passwd = spwd->sp_pwdp;
#ifdef	ATT_AGE
		pwd->pw_age = sptopwage (spwd);
#endif
	}
#endif
	return pwd;
}

/*
 * getpwnam - locate the password entry for a given name
 *
 * getpwnam() locates the first password file entry for the given name.
 * If there is a valid DBM file, the DBM files are queried first for
 * the entry.  Otherwise, a linear search is begun of the password file
 * searching for an entry which matches the provided name.
 */

struct passwd *
getpwnam (name)
char	*name;
{
	struct	passwd	*pwd;
#if defined(DBM) || defined(NDBM)
	datum	key;
	datum	content;
#endif
#ifdef	AUTOSHADOW
	struct	spwd	*spwd;
#endif

	if (setpwent ())
		return 0;

#if defined(DBM) || defined(NDBM)

	/*
	 * If the DBM file are now open, create a key for this UID and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		key.dsize = strlen (name);
		key.dptr = name;
#ifdef	DBM
		content = fetch (key);
#endif
#ifdef	NDBM
		content = dbm_fetch (pw_dbm, key);
#endif
		if (content.dptr != 0) {
			memcpy (pwdbuf, content.dptr, content.dsize);
			pw_unpack (pwdbuf, content.dsize, &pwent);
#ifdef	AUTOSHADOW
			if (spwd = getspnam (pwent.pw_name)) {
				pwent.pw_passwd = spwd->sp_pwdp;
#ifdef	ATT_AGE
				pwent.pw_age = sptopwage (spwd);
#endif
			}
#endif
			return &pwent;
		}
	}
#endif
	/*
	 * Search for an entry which matches the name.  Return the
	 * entry when a match is found.
	 */

	while (pwd = getpwent ())
		if (strcmp (pwd->pw_name, name) == 0)
			break;

#ifdef	AUTOSHADOW
	if (pwd && (spwd = getspnam (pwd->pw_name))) {
		pwd->pw_passwd = spwd->sp_pwdp;
#ifdef	ATT_AGE
		pwd->pw_age = sptopwage (spwd);
#endif
	}
#endif
	return pwd;
}

/*
 * setpwent - open the password file
 *
 * setpwent() opens the system password file, and the DBM password files
 * if they are present.  The system password file is rewound if it was
 * open already.
 */

int
setpwent ()
{
#ifdef	NDBM
	int	mode;
#endif

	if (! pwdfp) {
		if (! (pwdfp = fopen (pwdfile, "r")))
			return -1;
	} else {
		if (fseek (pwdfp, 0L, 0) != 0)
			return -1;
	}

	/*
	 * Attempt to open the DBM files if they have never been opened
	 * and an error has never been returned.
	 */

#if defined (DBM) || defined (NDBM)
	if (! dbmerror && ! dbmopened) {
		char	dbmfiles[BUFSIZ];

		strcpy (dbmfiles, pwdfile);
		strcat (dbmfiles, ".pag");
#ifdef	NDBM
		if (pw_dbm_mode == -1)
			mode = O_RDONLY;
		else
			mode = (pw_dbm_mode == O_RDONLY ||
				pw_dbm_mode == O_RDWR) ? pw_dbm_mode:O_RDONLY;
#endif
#ifdef	DBM
		if (access (dbmfiles, 0) || dbminit (pwdfile))
#endif
#ifdef	NDBM
		if (access (dbmfiles, 0) ||
			(! (pw_dbm = dbm_open (pwdfile, mode, 0))))
#endif
			dbmerror = 1;
		else
			dbmopened = 1;
	}
#endif
	return 0;
}

#endif /* GETPWENT */

#ifdef NEED_PUTPWENT

/*
 * putpwent - Output a (struct passwd) in character format
 *
 *	putpwent() writes out a (struct passwd) in the format it appears
 *	in in flat ASCII files.
 *
 *	(Author: Dr. Micheal Newberry)
 */

int
putpwent (p, f)
struct	passwd	*p;
FILE	*f;
{
	int	status;

#if defined(SUN) || defined(BSD) || defined(SUN4)
	status = fprintf (f, "%s:%s:%d:%d:%s,%s:%s:%s\n",
		p->pw_name, p->pw_passwd, p->pw_uid, p->pw_gid,
		p->pw_gecos, p->pw_comment, p->pw_dir, p->pw_shell) == EOF;
#else
	status = fprintf (f, "%s:%s", p->pw_name, p->pw_passwd) == EOF;
#ifdef	ATT_AGE
	if (p->pw_age && p->pw_age[0])
		status |= fprintf (f, ",%s", p->pw_age) == EOF;
#endif
	status |= fprintf (f, ":%d:%d:%s", p->pw_uid, p->pw_gid,
		p->pw_gecos) == EOF;
#ifdef	ATT_COMMENT
	if (p->pw_comment && p->pw_comment[0])
		status |= fprintf (f, ",%s", p->pw_comment) == EOF;
#endif
	status |= fprintf (f, ":%s:%s\n", p->pw_dir, p->pw_shell) == EOF;
#endif
	return status;
}
#endif /* NEED_PUTPWENT */
