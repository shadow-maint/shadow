/*
 * Copyright 1990, 1991, 1992, John F. Haugh II
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

#ifndef	lint
static	char	sccsid[] = "@(#)gsdbm.c	3.6	11:32:14	28 Jul 1992";
#endif

#include <string.h>
#include <stdio.h>
#include "shadow.h"
#include "config.h"

#ifdef	NDBM
#include <ndbm.h>
DBM	*sg_dbm;

#define	GRP_FRAG	256

/*
 * sg_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
sg_dbm_update (sgr)
struct	sgrp	*sgr;
{
	datum	key;
	datum	content;
	char	data[BUFSIZ*8];
	char	sgrpkey[60];
	char	*cp;
	int	len;
	int	i;
	int	cnt;
	static	int	once;

	if (! once) {
		if (! sg_dbm)
			setsgent ();

		once++;
	}
	if (! sg_dbm)
		return 0;

	len = sgr_pack (sgr, data);

	if (len <= GRP_FRAG) {
		content.dsize = len;
		content.dptr = data;

		key.dsize = strlen (sgr->sg_name);
		key.dptr = sgr->sg_name;
		if (dbm_store (sg_dbm, key, content, DBM_REPLACE))
			return 0;
	} else {
		content.dsize = sizeof cnt;
		content.dptr = (char *) &cnt;
		cnt = (len + (GRP_FRAG-1)) / GRP_FRAG;

		key.dsize = strlen (sgr->sg_name);
		key.dptr = sgr->sg_name;
		if (dbm_store (sg_dbm, key, content, DBM_REPLACE))
			return 0;

		for (cp = data, i = 0;i < cnt;i++) {
			content.dsize = len > GRP_FRAG ? GRP_FRAG:len;
			len -= content.dsize;
			content.dptr = cp;
			cp += content.dsize;

			key.dsize = sizeof i + strlen (sgr->sg_name);
			key.dptr = sgrpkey;
			memcpy (sgrpkey, (char *) &i, sizeof i);
			strcpy (sgrpkey + sizeof i, sgr->sg_name);
			if (dbm_store (sg_dbm, key, content, DBM_REPLACE))
				return 0;
		}
	}
	return 1;
}

/*
 * sg_dbm_remove
 *
 * Deletes the DBM shadow group file entries, if they exist.
 */

int
sg_dbm_remove (name)
char	*name;
{
	datum	key;
	datum	content;
	char	grpkey[60];
	int	i;
	int	cnt;
	int	errors = 0;
	static	int	once;

	if (! once) {
		if (! sg_dbm)
			setsgent ();

		once++;
	}
	if (! sg_dbm)
		return 0;

	key.dsize = strlen (name);
	key.dptr = name;
	content = dbm_fetch (sg_dbm, key);
	if (content.dptr == 0)
		++errors;
	else {
		if (content.dsize == sizeof (int)) {
			memcpy ((char *) &cnt, content.dptr, sizeof cnt);

			for (i = 0;i < cnt;i++) {
				key.dsize = sizeof i + strlen (name);
				key.dptr = grpkey;
				memcpy (grpkey, (char *) &i, sizeof i);
				strcpy (grpkey + sizeof i, name);
				if (dbm_delete (sg_dbm, key))
					++errors;
			}
		} else {
			if (dbm_delete (sg_dbm, key))
				++errors;
		}
	}
	return errors ? 0:1;
}
#endif
