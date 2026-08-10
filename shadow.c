/*
 * Copyright 1989, 1990, 1991, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include "shadow.h"
#include "config.h"
#include <stdio.h>

#ifdef	STDLIB_H
#include <stdlib.h>
#endif

#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif

#ifdef	NDBM
#include <ndbm.h>
#include <fcntl.h>
DBM	*sp_dbm;
int	sp_dbm_mode = -1;
static	int	dbmopened;
static	int	dbmerror;
#endif

#ifndef	lint
static	char	sccsid[] = "@(#)shadow.c	3.13	08:07:15	19 Jul 1993";
#endif

static	FILE	*shadow;
static	char	spwbuf[BUFSIZ];
static	struct	spwd	spwd;

#define	FIELDS	9
#define	OFIELDS	5

void
setspent ()
{
	if (shadow)
		rewind (shadow);
	else
		shadow = fopen (SHADOW, "r");

	/*
	 * Attempt to open the DBM files if they have never been opened
	 * and an error has never been returned.
	 */

#ifdef NDBM
	if (! dbmerror && ! dbmopened) {
		int	mode;
		char	dbmfiles[BUFSIZ];

		strcpy (dbmfiles, SHADOW);
		strcat (dbmfiles, ".pag");

		if (sp_dbm_mode == -1)
			mode = O_RDWR;
		else
			mode = (sp_dbm_mode == O_RDWR) ? O_RDWR:O_RDONLY;

		if (! (sp_dbm = dbm_open (SHADOW, mode, 0)))
			dbmerror = 1;
		else
			dbmopened = 1;
	}
#endif
}

void
endspent ()
{
	if (shadow)
		(void) fclose (shadow);

	shadow = (FILE *) 0;
#ifdef	NDBM
	if (dbmopened && sp_dbm) {
		dbm_close (sp_dbm);
		sp_dbm = 0;
	}
	dbmopened = 0;
	dbmerror = 0;
#endif
}

struct spwd *
sgetspent (string)
char	*string;
{
	char	*fields[FIELDS];
	char	*cp;
	char	*cpp;
	int	atoi ();
	long	atol ();
	int	i;

	strncpy (spwbuf, string, BUFSIZ-1);
	spwbuf[BUFSIZ-1] = '\0';

	if (cp = strrchr (spwbuf, '\n'))
		*cp = '\0';

	for (cp = spwbuf, i = 0;*cp && i < FIELDS;i++) {
		fields[i] = cp;
		while (*cp && *cp != ':')
			cp++;

		if (*cp)
			*cp++ = '\0';
	}
	if (i == (FIELDS-1))
		fields[i++] = cp;

	if ((cp && *cp) || (i != FIELDS && i != OFIELDS))
		return 0;

	spwd.sp_namp = fields[0];
	spwd.sp_pwdp = fields[1];

	if ((spwd.sp_lstchg = strtol (fields[2], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[2][0] == '\0')
		spwd.sp_lstchg = -1;

	if ((spwd.sp_min = strtol (fields[3], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[3][0] == '\0')
		spwd.sp_min = -1;

	if ((spwd.sp_max = strtol (fields[4], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[4][0] == '\0')
		spwd.sp_max = -1;

	if (i == OFIELDS) {
		spwd.sp_warn = spwd.sp_inact = spwd.sp_expire =
			spwd.sp_flag = -1;

		return &spwd;
	}
	if ((spwd.sp_warn = strtol (fields[5], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[5][0] == '\0')
		spwd.sp_warn = -1;

	if ((spwd.sp_inact = strtol (fields[6], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[6][0] == '\0')
		spwd.sp_inact = -1;

	if ((spwd.sp_expire = strtol (fields[7], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[7][0] == '\0')
		spwd.sp_expire = -1;

	if ((spwd.sp_flag = strtol (fields[8], &cpp, 10)) == 0 && *cpp)
		return 0;
	else if (fields[8][0] == '\0')
		spwd.sp_flag = -1;

	return (&spwd);
}

struct spwd
*fgetspent (fp)
FILE	*fp;
{
	char	buf[BUFSIZ];

	if (! fp)
		return (0);

	if (fgets (buf, BUFSIZ, fp) == (char *) 0)
		return (0);

	return sgetspent (buf);
}

struct spwd
*getspent ()
{
	if (! shadow)
		setspent ();

	return (fgetspent (shadow));
}

struct spwd
*getspnam (name)
#if	__STDC__
const
#endif
char	*name;
{
	struct	spwd	*sp;
#ifdef NDBM
	datum	key;
	datum	content;
#endif

	setspent ();

#ifdef NDBM

	/*
	 * If the DBM file are now open, create a key for this UID and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		key.dsize = strlen (name);
		key.dptr = name;

		content = dbm_fetch (sp_dbm, key);
		if (content.dptr != 0) {
			memcpy (spwbuf, content.dptr, content.dsize);
			spw_unpack (spwbuf, content.dsize, &spwd);
			return &spwd;
		}
	}
#endif
	while ((sp = getspent ()) != (struct spwd *) 0) {
		if (strcmp (name, sp->sp_namp) == 0)
			return (sp);
	}
	return (0);
}

int
putspent (sp, fp)
#if	__STDC__
const
#endif
struct	spwd	*sp;
FILE	*fp;
{
	int	errors = 0;

	if (! fp || ! sp)
		return -1;

	if (fprintf (fp, "%s:%s:", sp->sp_namp, sp->sp_pwdp) < 0)
		errors++;

	if (sp->sp_lstchg != -1) {
		if (fprintf (fp, "%ld:", sp->sp_lstchg) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_min != -1) {
		if (fprintf (fp, "%ld:", sp->sp_min) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_max != -1) {
		if (fprintf (fp, "%ld:", sp->sp_max) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_warn != -1) {
		if (fprintf (fp, "%ld:", sp->sp_warn) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_inact != -1) {
		if (fprintf (fp, "%ld:", sp->sp_inact) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_expire != -1) {
		if (fprintf (fp, "%ld:", sp->sp_expire) < 0)
			errors++;
	} else if (putc (':', fp) == EOF)
		errors++;

	if (sp->sp_flag != -1) {
		if (fprintf (fp, "%ld", sp->sp_flag) < 0)
			errors++;
	}
	if (putc ('\n', fp) == EOF)
		errors++;

	if (errors)
		return -1;
	else
		return 0;
}
