/*
 * Copyright 1990, 1991, John F. Haugh II
 * All rights reserved.
 *
 * Use, duplication, and disclosure prohibited without
 * the express written permission of the author.
 */

#ifndef	lint
static	char	sccsid[] = "@(#)spdbm.c	3.3	08:46:22	12 Sep 1991";
#endif

#include <string.h>
#include <stdio.h>
#include "config.h"
#include "shadow.h"

#ifdef	NDBM
#include <ndbm.h>
DBM	*sp_dbm;

/*
 * sp_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
sp_dbm_update (sp)
struct	spwd	*sp;
{
	datum	key;
	datum	content;
	char	data[BUFSIZ];
	int	len;
	static	int	once;

	if (! once) {
		if (! sp_dbm)
			setspent ();

		once++;
	}
	if (! sp_dbm)
		return 0;

	len = spw_pack (sp, data);

	content.dsize = len;
	content.dptr = data;

	key.dsize = strlen (sp->sp_namp);
	key.dptr = sp->sp_namp;
	if (dbm_store (sp_dbm, key, content, DBM_REPLACE))
		return 0;

	return 1;
}

/*
 * sp_dbm_remove
 *
 * Updates the DBM password files, if they exist.
 */

int
sp_dbm_remove (user)
char	*user;
{
	datum	key;
	static	int	once;

	if (! once) {
		if (! sp_dbm)
			setspent ();

		once++;
	}
	if (! sp_dbm)
		return 0;

	key.dsize = strlen (user);
	key.dptr = user;
	if (dbm_delete (sp_dbm, key))
		return 0;

	return 1;
}
#endif
