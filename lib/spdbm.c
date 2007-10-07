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

#if defined(SHADOWPWD) && defined(NDBM)	/*{*/

#include "rcsid.h"
RCSID("$Id: spdbm.c,v 1.3 1997/12/07 23:26:58 marekm Exp $")

#include <string.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"

#include <ndbm.h>
extern	DBM	*sp_dbm;

/*
 * sp_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
sp_dbm_update(struct spwd *sp)
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
sp_dbm_remove(char *user)
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

int
sp_dbm_present(void)
{
	return (access(SHADOW_PAG_FILE, F_OK) == 0);
}
#endif	/*} SHADOWPWD && NDBM */
