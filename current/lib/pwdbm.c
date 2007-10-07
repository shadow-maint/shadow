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

#ifdef NDBM /*{*/

#include "rcsid.h"
RCSID("$Id: pwdbm.c,v 1.4 1997/12/14 20:07:19 marekm Exp $")

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include "prototypes.h"
#include "defines.h"

#include <ndbm.h>
extern DBM *pw_dbm;

/*
 * pw_dbm_update
 *
 * Updates the DBM password files, if they exist.
 */

int
pw_dbm_update(const struct passwd *pw)
{
	datum	key;
	datum	content;
	char	data[BUFSIZ];
	int	len;
	static	int	once;

	if (! once) {
		if (! pw_dbm)
			setpwent ();
		once++;
	}
	if (! pw_dbm)
		return 0;

	len = pw_pack (pw, data);
	content.dsize = len;
	content.dptr = data;

	key.dsize = strlen (pw->pw_name);
	key.dptr = pw->pw_name;

	if (dbm_store(pw_dbm, key, content, DBM_REPLACE))
		return 0;

	/*
	 * XXX - on systems with 16-bit UIDs (such as Linux/x86)
	 * name "aa" and UID 24929 will give the same key.  This
	 * happens only rarely, but code which only "works most
	 * of the time" is not good enough...
	 *
	 * This needs to be fixed in several places (pwdbm.c,
	 * grdbm.c, pwent.c, grent.c).  Fixing it will cause
	 * incompatibility with existing dbm files.
	 *
	 * Summary: don't use this stuff for now.  --marekm
	 */

	key.dsize = sizeof pw->pw_uid;
	key.dptr = (char *) &pw->pw_uid;

	if (dbm_store(pw_dbm, key, content, DBM_REPLACE))
		return 0;

	return 1;
}

/*
 * pw_dbm_remove
 *
 * Removes the DBM password entry, if it exists.
 */

int
pw_dbm_remove(const struct passwd *pw)
{
	datum	key;
	static	int	once;
	char	data[BUFSIZ];

	if (! once) {
		if (! pw_dbm)
			setpwent ();
		once++;
	}
	if (! pw_dbm)
		return 0;

	key.dsize = strlen (pw->pw_name);
	key.dptr = pw->pw_name;

	if (dbm_delete (pw_dbm, key))
		return 0;

	key.dsize = sizeof pw->pw_uid;
	key.dptr = (char *) &pw->pw_uid;

	if (dbm_delete (pw_dbm, key))
		return 0;

	return 1;
}


int
pw_dbm_present(void)
{
	return (access(PASSWD_PAG_FILE, F_OK) == 0);
}
#endif	/* NDBM */
