// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/shadow/sgetspent.h"

#ifndef HAVE_SGETSPENT

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "atoi/a2i.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "sizeof.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#define	FIELDS	9
#define	OFIELDS	5


/*
 * sgetspent - convert string in shadow file format to (struct spwd *)
 */
struct spwd *
sgetspent(const char *s)
{
	static char        *dup = NULL;
	static struct spwd spwd;

	char *fields[FIELDS];
	size_t  i;

	free(dup);
	dup = strdup(s);
	if (dup == NULL)
		return NULL;

	stpsep(dup, "\n");

	i = strsep2arr(dup, ":", countof(fields), fields);
	if (i == countof(fields) - 1)
		fields[i++] = "";
	if (i != countof(fields) && i != OFIELDS)
		return NULL;

	/*
	 * Start populating the structure.  The fields are all in
	 * static storage, as is the structure we pass back.
	 */

	spwd.sp_namp = fields[0];
	spwd.sp_pwdp = fields[1];
	if (streq(fields[2], "0"))
		spwd.sp_lstchg = 0;
	else
		spwd.sp_lstchg = -1;
	spwd.sp_min = -1;
	spwd.sp_max = -1;

	/*
	 * If there are only OFIELDS fields (this is a SVR3.2 /etc/shadow
	 * formatted file), initialize the other field members to -1.
	 */

	if (i == OFIELDS) {
		spwd.sp_warn   = -1;
		spwd.sp_inact  = -1;
		spwd.sp_expire = -1;
		spwd.sp_flag   = SHADOW_SP_FLAG_UNSET;

		return &spwd;
	}

	spwd.sp_warn = -1;
	spwd.sp_inact = -1;

	/*
	 * Get the number of days after the epoch before the account is
	 * set to expire.
	 */

	if (streq(fields[7], ""))
		spwd.sp_expire = -1;
	else if (a2sl(&spwd.sp_expire, fields[7], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

	/*
	 * This field is reserved for future use.  But it isn't supposed
	 * to have anything other than a valid integer in it.
	 */

	if (streq(fields[8], ""))
		spwd.sp_flag = SHADOW_SP_FLAG_UNSET;
	else if (str2ul(&spwd.sp_flag, fields[8]) == -1)
		return NULL;

	return (&spwd);
}
#else
extern int ISO_C_forbids_an_empty_translation_unit;
#endif
