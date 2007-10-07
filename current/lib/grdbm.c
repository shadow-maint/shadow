/*
 * Copyright 1990 - 1994, Julianne Frances Haugh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#ifdef	NDBM

#include "rcsid.h"
RCSID("$Id: grdbm.c,v 1.3 1997/12/07 23:26:52 marekm Exp $")

#include <string.h>
#include <stdio.h>
#include <grp.h>
#include "prototypes.h"

#include <ndbm.h>
extern	DBM	*gr_dbm;

#define	GRP_FRAG	256

/*
 * gr_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
gr_dbm_update(const struct group *gr)
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
gr_dbm_remove(const struct group *gr)
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

int
gr_dbm_present(void)
{
	return (access(GROUP_PAG_FILE, F_OK) == 0);
}
#endif
