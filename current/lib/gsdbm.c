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

#if defined(NDBM) && defined(SHADOWGRP) /*{*/

#include <string.h>
#include <stdio.h>
#include "prototypes.h"

#include "rcsid.h"
RCSID("$Id: gsdbm.c,v 1.3 1997/12/07 23:26:53 marekm Exp $")

#include <ndbm.h>
extern	DBM	*sg_dbm;

#define	GRP_FRAG	256

/*
 * sg_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
sg_dbm_update(const struct sgrp *sgr)
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
sg_dbm_remove(const char *name)
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

int
sg_dbm_present(void)
{
	return (access(SGROUP_PAG_FILE, F_OK) == 0);
}
#endif /*} SHADOWGRP && NDBM */
