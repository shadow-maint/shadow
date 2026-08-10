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

#include <stdio.h>
#include <grp.h>
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else	/* !BSD */
#include <string.h>
#endif	/* BSD */
#include "config.h"

#ifdef	AUTOSHADOW
#include "shadow.h"
#endif	/* AUTOSHADOW */

#ifdef	NDBM
#include <ndbm.h>
#include <fcntl.h>
DBM	*gr_dbm;
int	gr_dbm_mode = -1;
#endif	/* NDBM */

#ifndef	lint
static	char	sccsid[] = "@(#)grent.c	3.13	08:57:37	10 Jun 1993";
#endif	/* !lint */

#define	NFIELDS	4
#define	MAXMEM	1024

static	char	grpbuf[4*BUFSIZ];
static	char	*grpfields[NFIELDS];
static	char	*members[MAXMEM+1];
static	struct	group	grent;

static	FILE	*grpfp;
static	char	*grpfile = GRPFILE;
#ifdef	NDBM
static	int	dbmopened;
static	int	dbmerror;
#endif	/* NDBM */

char *
fgetsx (buf, cnt, f)
char	*buf;
int	cnt;
FILE	*f;
{
	char	*cp = buf;
	char	*ep;

	while (cnt > 0) {
		if (fgets (cp, cnt, f) == 0)
			if (cp == buf)
				return 0;
			else
				break;

		if ((ep = strrchr (cp, '\\')) && *(ep + 1) == '\n') {
			if ((cnt -= ep - cp) > 0)
				*(cp = ep) = '\0';
		} else
			break;
	}
	return buf;
}

int
fputsx (s, stream)
char	*s;
FILE	*stream;
{
	int	i;

	for (i = 0;*s;i++, s++) {
		if (putc (*s, stream) == EOF)
			return EOF;

		if (i > (BUFSIZ/2)) {
			if (putc ('\\', stream) == EOF ||
			    putc ('\n', stream) == EOF)
				return EOF;

			i = 0;
		}
	}
	return 0;
}

/*
 * list - turn a comma-separated string into an array of (char *)'s
 *
 *	list() converts the comma-separated list of member names into
 *	an array of character pointers.
 *
 *	WARNING: I profiled this once with and without strchr() calls
 *	and found that using a register variable and an explicit loop
 *	works best.  For large /etc/group files, this is a major win.
 */

static char **
list (s)
register char	*s;
{
	int	nmembers = 0;

	while (s && *s) {
		members[nmembers++] = s;
		while (*s && *s != ',')
			s++;

		if (*s)
			*s++ = '\0';
	}
	members[nmembers] = (char *) 0;
	return members;
}

struct	group	*sgetgrent (buf)
char	*buf;
{
	int	i;
	char	*cp;

	strncpy (grpbuf, buf, sizeof grpbuf);
	grpbuf[sizeof grpbuf - 1] = '\0';
	if (cp = strrchr (grpbuf, '\n'))
		*cp = '\0';

	for (cp = grpbuf, i = 0;i < NFIELDS && cp;i++) {
		grpfields[i] = cp;
		if (cp = strchr (cp, ':'))
			*cp++ = 0;
	}
	if (i < (NFIELDS-1) || *grpfields[2] == '\0')
		return ((struct group *) 0);

	grent.gr_name = grpfields[0];
	grent.gr_passwd = grpfields[1];
	grent.gr_gid = atoi (grpfields[2]);
	grent.gr_mem = list (grpfields[3]);

	return (&grent);
}

int
putgrent (g, f)
struct	group	*g;
FILE	*f;
{
	int	i;
	char	*cp;
	char	buf[BUFSIZ*4];

	if (! g || ! f)
		return -1;

	sprintf (buf, "%s:%s:%d:", g->gr_name, g->gr_passwd, g->gr_gid);
	if (g->gr_mem) {
		cp = strchr (buf, '\0');
		for (i = 0;g->gr_mem[i];i++) {
			if ((cp - buf) + strlen (g->gr_mem[i]) + 2
					>= sizeof buf)
				return -1;

			if (i > 0) {
				strcpy (cp, ",");
				cp++;
			}
			strcpy (cp, g->gr_mem[i]);
			cp = strchr (cp, '\0');
		}
		strcat (cp, "\n");
	} else
		strcat (buf, "\n");

	if (fputsx (buf, f) == EOF || ferror (f))
		return -1;

	return 0;
}

#ifdef	GETGRENT

/*
 * fgetgrent - get a group file entry from a stream
 *
 * fgetgrent() reads the next line from a group file formatted stream
 * and returns a pointer to the group structure for that line.
 */

struct	group	*fgetgrent (fp)
FILE	*fp;
{
	char	buf[BUFSIZ*4];
	char	*cp;

	if (fgetsx (buf, sizeof buf, fp) != (char *) 0) {
		if (cp = strchr (buf, '\n'))
			*cp = '\0';

		return (sgetgrent (buf));
	}
	return 0;
}

/*
 * endgrent - close a group file
 *
 * endgrent() closes the group file if open.
 */

#ifdef	SVR4
void
#else
int
#endif
endgrent ()
{
	if (grpfp)
		if (fclose (grpfp))
#ifdef	SVR4
			return;
#else
			return -1;
#endif
	grpfp = 0;
#ifdef	NDBM
	if (dbmopened && gr_dbm) {
		dbm_close (gr_dbm);
		gr_dbm = 0;
	}
	dbmopened = 0;
	dbmerror = 0;
#endif	/* NDBM */
#ifndef	SVR4
	return 0;
#endif
}

/*
 * getgrent - get a group entry from the group file
 *
 * getgrent() opens the group file, if not already opened, and reads
 * a single entry.  NULL is returned if any errors are encountered reading
 * the group file.
 */

struct	group	*getgrent ()
{
#ifdef	SVR4
	if (! grpfp)
		setgrent ();
#else
	if (! grpfp && setgrent ())
		return 0;
#endif
	return fgetgrent (grpfp);
}

/*
 * getgrgid - locate the group entry for a given GID
 *
 * getgrgid() locates the first group file entry for the given GID.
 * If there is a valid DBM file, the DBM files are queried first for
 * the entry.  Otherwise, a linear search is begun of the group file
 * searching for an entry which matches the provided GID.
 */

struct	group	*getgrgid (gid)
GID_T	gid;
{
	struct	group	*grp;
#ifdef NDBM
	datum	key;
	datum	content;
	int	cnt;
	int	i;
	char	*cp;
	char	grpkey[64];
#endif	/* NDBM */
#ifdef	AUTOSHADOW
	struct	sgrp	*sgrp;
#endif	/* AUTOSHADOW */

#ifdef	SVR4
	setgrent ();
#else
	if (setgrent ())
		return 0;
#endif
#ifdef NDBM

	/*
	 * If the DBM file are now open, create a key for this GID and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		grent.gr_gid = gid;
		key.dsize = sizeof grent.gr_gid;
		key.dptr = (char *) &grent.gr_gid;
		content = dbm_fetch (gr_dbm, key);
		if (content.dptr == 0)
			return 0;

		if (content.dsize == sizeof (int)) {
			memcpy ((char *) &cnt, content.dptr, content.dsize);
			for (cp = grpbuf, i = 0;i < cnt;i++) {
				memcpy (grpkey, (char *) &i, (int) sizeof i);
				memcpy (grpkey + sizeof i,
					(char *) &grent.gr_gid,
					(int) sizeof grent.gr_gid);

				key.dsize = sizeof i + sizeof grent.gr_gid;
				key.dptr = grpkey;

				content = dbm_fetch (gr_dbm, key);
				if (content.dptr == 0)
					return 0;

				memcpy (cp, content.dptr, content.dsize);
				cp += content.dsize;
			}
			grent.gr_mem = members;
			gr_unpack (grpbuf, cp - grpbuf, &grent);
#ifdef	AUTOSHADOW
			if (sgrp = getsgnam (grent.gr_name)) {
				grent.gr_passwd = sgrp->sg_passwd;
				grent.gr_mem = sgrp->sg_mem;
			}
#endif	/* AUTOSHADOW */
			return &grent;
		} else {
			grent.gr_mem = members;
			memcpy (grpbuf, content.dptr, content.dsize);
			gr_unpack (grpbuf, content.dsize, &grent);
#ifdef	AUTOSHADOW
			if (sgrp = getsgnam (grent.gr_name)) {
				grent.gr_passwd = sgrp->sg_passwd;
				grent.gr_mem = sgrp->sg_mem;
			}
#endif	/* AUTOSHADOW */
			return &grent;
		}
	}
#endif	/* NDBM */
	/*
	 * Search for an entry which matches the GID.  Return the
	 * entry when a match is found.
	 */

	while (grp = getgrent ())
		if (grp->gr_gid == gid)
			break;

#ifdef	AUTOSHADOW
	if (grp) {
		if (sgrp = getsgnam (grent.gr_name)) {
			grp->gr_passwd = sgrp->sg_passwd;
			grp->gr_mem = sgrp->sg_mem;
		}
	}
#endif	/* AUTOSHADOW */
	return grp;
}

/*
 * getgrnam - locate the group entry for a given name
 *
 * getgrnam() locates the first group file entry for the given name.
 * If there is a valid DBM file, the DBM files are queried first for
 * the entry.  Otherwise, a linear search is begun of the group file
 * searching for an entry which matches the provided name.
 */

struct	group	*getgrnam (name)
#if	__STDC__
const	char	*name;
#else
char	*name;
#endif
{
	struct	group	*grp;
#ifdef NDBM
	datum	key;
	datum	content;
	int	cnt;
	int	i;
	char	*cp;
	char	grpkey[64];
#endif	/* NDBM */
#ifdef	AUTOSHADOW
	struct	sgrp	*sgrp;
#endif	/* AUTOSHADOW */

#ifdef	SVR4
	setgrent ();
#else
	if (setgrent ())
		return 0;
#endif
#ifdef NDBM

	/*
	 * If the DBM file are now open, create a key for this GID and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		key.dsize = strlen (name);
		key.dptr = name;
		content = dbm_fetch (gr_dbm, key);
		if (content.dptr == 0)
			return 0;

		if (content.dsize == sizeof (int)) {
			memcpy ((char *) &cnt, content.dptr, content.dsize);
			for (cp = grpbuf, i = 0;i < cnt;i++) {
				memcpy (grpkey, (char *) &i, (int) sizeof i);
				strcpy (grpkey + sizeof i, name);

				key.dsize = sizeof i + strlen (name);
				key.dptr = grpkey;

				content = dbm_fetch (gr_dbm, key);
				if (content.dptr == 0)
					return 0;

				memcpy (cp, content.dptr, content.dsize);
				cp += content.dsize;
			}
			grent.gr_mem = members;
			gr_unpack (grpbuf, cp - grpbuf, &grent);
#ifdef	AUTOSHADOW
			if (sgrp = getsgnam (grent.gr_name)) {
				grent.gr_passwd = sgrp->sg_passwd;
				grent.gr_mem = sgrp->sg_mem;
			}
#endif	/* AUTOSHADOW */
			return &grent;
		} else {
			grent.gr_mem = members;
			memcpy (grpbuf, content.dptr, content.dsize);
			gr_unpack (grpbuf, content.dsize, &grent);
#ifdef	AUTOSHADOW
			if (sgrp = getsgnam (grent.gr_name)) {
				grent.gr_passwd = sgrp->sg_passwd;
				grent.gr_mem = sgrp->sg_mem;
			}
#endif	/* AUTOSHADOW */
			return &grent;
		}
	}
#endif	/* NDBM */
	/*
	 * Search for an entry which matches the name.  Return the
	 * entry when a match is found.
	 */

	while (grp = getgrent ())
		if (strcmp (grp->gr_name, name) == 0)
			break;

#ifdef	AUTOSHADOW
	if (grp) {
		if (sgrp = getsgnam (grent.gr_name)) {
			grp->gr_passwd = sgrp->sg_passwd;
			grp->gr_mem = sgrp->sg_mem;
		}
	}
#endif	/* AUTOSHADOW */
	return grp;
}

/*
 * setgrent - open the group file
 *
 * setgrent() opens the system group file, and the DBM group files
 * if they are present.  The system group file is rewound if it was
 * open already.
 */

#ifdef	SVR4
void
#else
int
#endif
setgrent ()
{
#ifdef	NDBM
	int	mode;
#endif	/* NDBM */

	if (! grpfp) {
		if (! (grpfp = fopen (grpfile, "r")))
#ifdef	SVR4
			return;
#else
			return -1;
#endif
	} else {
		if (fseek (grpfp, 0L, 0) != 0)
#ifdef	SVR4
			return;
#else
			return -1;
#endif
	}

	/*
	 * Attempt to open the DBM files if they have never been opened
	 * and an error has never been returned.
	 */

#ifdef NDBM
	if (! dbmerror && ! dbmopened) {
		char	dbmfiles[BUFSIZ];

		strcpy (dbmfiles, grpfile);
		strcat (dbmfiles, ".pag");
		if (gr_dbm_mode == -1)
			mode = O_RDONLY;
		else
			mode = (gr_dbm_mode == O_RDONLY ||
				gr_dbm_mode == O_RDWR) ? gr_dbm_mode:O_RDONLY;

		if (access (dbmfiles, 0) ||
			(! (gr_dbm = dbm_open (grpfile, mode, 0))))
			dbmerror = 1;
		else
			dbmopened = 1;
	}
#endif	/* NDBM */
#ifndef	SVR4
	return 0;
#endif
}

#endif	/* GRENT */
