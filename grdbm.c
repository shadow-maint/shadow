/*
 * Copyright 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 */

#ifndef	lint
static	char	sccsid[] = "@(#)grdbm.c	3.3	08:44:03	12 Sep 1991";
#endif

#include <string.h>
#include <stdio.h>
#include <grp.h>
#include "config.h"

#ifdef	NDBM
#include <ndbm.h>
DBM	*gr_dbm;

#define	GRP_FRAG	256

/*
 * gr_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
gr_dbm_update (gr)
struct	group	*gr;
{
	datum	key;
	datum	content;
	char	data[BUFSIZ*8];
	char	grpkey[60];
	char	*cp;
	int	len;
	int	i;
	int	cnt;
	static	int	once;

	if (! once) {
		if (! gr_dbm)
			setgrent ();

		once++;
	}
	if (! gr_dbm)
		return 0;

	len = gr_pack (gr, data);

	if (len <= GRP_FRAG) {
		content.dsize = len;
		content.dptr = data;

		key.dsize = strlen (gr->gr_name);
		key.dptr = gr->gr_name;
		if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
			return 0;

		key.dsize = sizeof gr->gr_gid;
		key.dptr = (char *) &gr->gr_gid;
		if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
			return 0;

	} else {
		content.dsize = sizeof cnt;
		content.dptr = (char *) &cnt;
		cnt = (len + (GRP_FRAG-1)) / GRP_FRAG;

		key.dsize = strlen (gr->gr_name);
		key.dptr = gr->gr_name;
		if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
			return 0;

		key.dsize = sizeof gr->gr_gid;
		key.dptr = (char *) &gr->gr_gid;
		if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
			return 0;

		for (cp = data, i = 0;i < cnt;i++) {
			content.dsize = len > GRP_FRAG ? GRP_FRAG:len;
			len -= content.dsize;
			content.dptr = cp;
			cp += content.dsize;

			key.dsize = sizeof i + strlen (gr->gr_name);
			key.dptr = grpkey;
			memcpy (grpkey, (char *) &i, sizeof i);
			strcpy (grpkey + sizeof i, gr->gr_name);
			if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
				return 0;

			key.dsize = sizeof i + sizeof gr->gr_gid;
			key.dptr = grpkey;
			memcpy (grpkey, (char *) &i, sizeof i);
			memcpy (grpkey + sizeof i, (char *) &gr->gr_gid,
				sizeof gr->gr_gid);
			if (dbm_store (gr_dbm, key, content, DBM_REPLACE))
				return 0;
		}
	}
	return 1;
}

/*
 * gr_dbm_remove
 *
 * Deletes the DBM group file entries, if they exist.
 */

int
gr_dbm_remove (gr)
struct	group	*gr;
{
	datum	key;
	datum	content;
	char	grpkey[60];
	int	i;
	int	cnt;
	int	errors = 0;
	static	int	once;

	if (! once) {
		if (! gr_dbm)
			setgrent ();

		once++;
	}
	if (! gr_dbm)
		return 0;

	key.dsize = strlen (gr->gr_name);
	key.dptr = (char *) gr->gr_name;
	content = dbm_fetch (gr_dbm, key);
	if (content.dptr == 0)
		++errors;
	else {
		if (content.dsize == sizeof (int)) {
			memcpy ((char *) &cnt, content.dptr, sizeof cnt);

			for (i = 0;i < cnt;i++) {
				key.dsize = sizeof i + strlen (gr->gr_name);
				key.dptr = grpkey;
				memcpy (grpkey, (char *) &i, sizeof i);
				strcpy (grpkey + sizeof i, gr->gr_name);
				if (dbm_delete (gr_dbm, key))
					++errors;
			}
		} else {
			if (dbm_delete (gr_dbm, key))
				++errors;
		}
	}
	key.dsize = sizeof gr->gr_gid;
	key.dptr = (char *) &gr->gr_gid;
	content = dbm_fetch (gr_dbm, key);
	if (content.dptr == 0)
		++errors;
	else {
		if (content.dsize == sizeof (int)) {
			memcpy ((char *) &cnt, content.dptr, sizeof cnt);

			for (i = 0;i < cnt;i++) {
				key.dsize = sizeof i + sizeof gr->gr_gid;
				key.dptr = grpkey;
				memcpy (grpkey, (char *) &i, sizeof i);
				memcpy (grpkey + sizeof i, (char *) &gr->gr_gid,
					sizeof gr->gr_gid);

				if (dbm_delete (gr_dbm, key))
					++errors;
			}
		} else {
			if (dbm_delete (gr_dbm, key))
				++errors;
		}
	}
	return errors ? 0:1;
}
#endif
