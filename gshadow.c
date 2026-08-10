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

#include "config.h"

#ifdef	SHADOWGRP

#include "shadow.h"
#include <stdio.h>
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
DBM	*sg_dbm;
int	sg_dbm_mode = -1;
static	int	dbmopened;
static	int	dbmerror;
#endif


#ifndef	lint
static	char	sccsid[] = "@(#)gshadow.c	3.9	08:57:38	10 Jun 1993";
#endif

#define	MAXMEM	1024

static	FILE	*shadow;
static	char	*sgrpfile = "/etc/gshadow";
static	char	sgrbuf[BUFSIZ*4];
static	char	*members[MAXMEM+1];
static	char	*admins[MAXMEM+1];
static	struct	sgrp	sgroup;

extern	char	*fgetsx();
extern	int	fputsx();

#define	FIELDS	4

static char **
list (s, l)
char	*s;
char	**l;
{
	int	nmembers = 0;

	while (s && *s) {
		l[nmembers++] = s;
		if (s = strchr (s, ','))
			*s++ = '\0';
	}
	l[nmembers] = (char *) 0;
	return l;
}

void
setsgent ()
{
#ifdef	NDBM
	int	mode;
#endif	/* NDBM */

	if (shadow)
		rewind (shadow);
	else
		shadow = fopen (GSHADOW, "r");

	/*
	 * Attempt to open the DBM files if they have never been opened
	 * and an error has never been returned.
	 */

#ifdef NDBM
	if (! dbmerror && ! dbmopened) {
		char	dbmfiles[BUFSIZ];

		strcpy (dbmfiles, sgrpfile);
		strcat (dbmfiles, ".pag");

		if (sg_dbm_mode == -1)
			mode = O_RDWR;
		else
			mode = (sg_dbm_mode == O_RDWR) ? O_RDWR:O_RDONLY;

		if (access (dbmfiles, 0) ||
			(! (sg_dbm = dbm_open (sgrpfile, mode, 0))))
			dbmerror = 1;
		else
			dbmopened = 1;
	}
#endif	/* NDBM */
}

void
endsgent ()
{
	if (shadow)
		(void) fclose (shadow);

	shadow = (FILE *) 0;
#ifdef	NDBM
	if (dbmopened && sg_dbm) {
		dbm_close (sg_dbm);
		dbmopened = 0;
		sg_dbm = 0;
	}
#endif
}

struct sgrp *
sgetsgent (string)
char	*string;
{
	char	*fields[FIELDS];
	char	*cp;
	int	atoi ();
	long	atol ();
	int	i;

	strncpy (sgrbuf, string, (int) sizeof sgrbuf - 1);
	sgrbuf[sizeof sgrbuf - 1] = '\0';

	if (cp = strrchr (sgrbuf, '\n'))
		*cp = '\0';

	/*
	 * There should be exactly 4 colon separated fields.  Find
	 * all 4 of them and save the starting addresses in fields[].
	 */

	for (cp = sgrbuf, i = 0;i < FIELDS && cp;i++) {
		fields[i] = cp;
		if (cp = strchr (cp, ':'))
			*cp++ = '\0';
	}

	/*
	 * If there was an extra field somehow, or perhaps not enough,
	 * the line is invalid.
	 */

	if (cp || i != FIELDS)
		return 0;

	sgroup.sg_name = fields[0];
	sgroup.sg_passwd = fields[1];
	sgroup.sg_adm = list (fields[2], admins);
	sgroup.sg_mem = list (fields[3], members);

	return &sgroup;
}

struct sgrp
*fgetsgent (fp)
FILE	*fp;
{
	char	buf[sizeof sgrbuf];

	if (! fp)
		return (0);

	if (fgetsx (buf, sizeof buf, fp) == (char *) 0)
		return (0);

	return sgetsgent (buf);
}

struct sgrp
*getsgent ()
{
	if (! shadow)
		setsgent ();

	return (fgetsgent (shadow));
}

struct sgrp *
getsgnam (name)
char	*name;
{
	struct	sgrp	*sgrp;
#ifdef NDBM
	datum	key;
	datum	content;
#endif

	setsgent ();

#ifdef NDBM

	/*
	 * If the DBM file are now open, create a key for this group and
	 * try to fetch the entry from the database.  A matching record
	 * will be unpacked into a static structure and returned to
	 * the user.
	 */

	if (dbmopened) {
		key.dsize = strlen (name);
		key.dptr = name;

		content = dbm_fetch (sg_dbm, key);
		if (content.dptr != 0) {
			memcpy (sgrbuf, content.dptr, content.dsize);
			sgroup.sg_mem = members;
			sgroup.sg_adm = admins;
			sgr_unpack (sgrbuf, content.dsize, &sgroup);
			return &sgroup;
		}
	}
#endif
	while ((sgrp = getsgent ()) != (struct sgrp *) 0) {
		if (strcmp (name, sgrp->sg_name) == 0)
			return (sgrp);
	}
	return (0);
}

int
putsgent (sgrp, fp)
struct	sgrp	*sgrp;
FILE	*fp;
{
	char	buf[sizeof sgrbuf];
	char	*cp = buf;
	int	i;

	if (! fp || ! sgrp)
		return -1;

	/*
	 * Copy the group name and passwd.
	 */

	strcpy (cp, sgrp->sg_name);
	cp += strlen (cp);
	*cp++ = ':';

	strcpy (cp, sgrp->sg_passwd);
	cp += strlen (cp);
	*cp++ = ':';

	/*
	 * Copy the administrators, separating each from the other
	 * with a ",".
	 */

	for (i = 0;sgrp->sg_adm[i];i++) {
		if (i > 0)
			*cp++ = ',';

		strcpy (cp, sgrp->sg_adm[i]);
		cp += strlen (cp);
	}
	*cp++ = ':';

	/*
	 * Now do likewise with the group members.
	 */

	for (i = 0;sgrp->sg_mem[i];i++) {
		if (i > 0)
			*cp++ = ',';

		strcpy (cp, sgrp->sg_mem[i]);
		cp += strlen (cp);
	}
	*cp++ = '\n';
	*cp = '\0';

	/*
	 * Output using the function which understands the line
	 * continuation conventions.
	 */

	return fputsx (buf, fp);
}

#endif	/* SHADOWGRP */
