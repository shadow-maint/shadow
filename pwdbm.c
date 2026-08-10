/*
 * Copyright 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#ifndef	lint
static	char	sccsid[] = "@(#)pwdbm.c	3.6	12:10:31	28 Dec 1991";
#endif

#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#endif
#include <stdio.h>
#include "pwd.h"
#include "config.h"

#if defined(DBM) || defined(NDBM) /*{*/

#ifdef	DBM
#include <dbm.h>
#endif
#ifdef	NDBM
#include <ndbm.h>
DBM	*pw_dbm;
#endif

/*
 * pw_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
pw_dbm_update (pw)
struct	passwd	*pw;
{
	datum	key;
	datum	content;
	char	data[BUFSIZ];
	int	len;
	static	int	once;

	if (! once) {
#ifdef	NDBM
		if (! pw_dbm)
			setpwent ();
#else
		setpwent ();
#endif
		once++;
	}
#ifdef	DBM
	strcpy (data, PWDFILE);
	strcat (data, ".pag");
	if (access (data, 0))
		return 0;
#endif
#ifdef	NDBM
	if (! pw_dbm)
		return 0;
#endif
	len = pw_pack (pw, data);
	content.dsize = len;
	content.dptr = data;

	key.dsize = strlen (pw->pw_name);
	key.dptr = pw->pw_name;
#ifdef	DBM
	if (store (key, content))
		return 0;
#endif
#ifdef	NDBM
	if (dbm_store (pw_dbm, key, content, DBM_REPLACE))
		return 0;
#endif

	key.dsize = sizeof pw->pw_uid;
	key.dptr = (char *) &pw->pw_uid;
#ifdef	DBM
	if (store (key, content))
		return 0;
#endif
#ifdef	NDBM
	if (dbm_store (pw_dbm, key, content, DBM_REPLACE))
		return 0;
#endif
	return 1;
}

/*
 * pw_dbm_remove
 *
 * Removes the DBM password entry, if it exists.
 */

int
pw_dbm_remove (pw)
struct	passwd	*pw;
{
	datum	key;
	static	int	once;
	char	data[BUFSIZ];

	if (! once) {
#ifdef	NDBM
		if (! pw_dbm)
			setpwent ();
#else
		setpwent ();
#endif
		once++;
	}
#ifdef	DBM
	strcpy (data, PWDFILE);
	strcat (data, ".pag");
	if (access (data, 0))
		return 0;
#endif
#ifdef	NDBM
	if (! pw_dbm)
		return 0;
#endif
	key.dsize = strlen (pw->pw_name);
	key.dptr = pw->pw_name;
#ifdef	DBM
	if (delete (key))
		return 0;
#endif
#ifdef	NDBM
	if (dbm_delete (pw_dbm, key))
		return 0;
#endif
	key.dsize = sizeof pw->pw_uid;
	key.dptr = (char *) &pw->pw_uid;
#ifdef	DBM
	if (delete (key))
		return 0;
#endif
#ifdef	NDBM
	if (dbm_delete (pw_dbm, key))
		return 0;
#endif
	return 1;
}

#endif	/*} defined(NDBM) || defined(DBM) */
