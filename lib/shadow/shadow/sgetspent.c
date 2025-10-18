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

#include "atoi/a2i/a2s.h"
#include "atoi/str2i.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "sizeof.h"
#include "string/strcmp/streq.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#define	FIELDS	9
#define	OFIELDS	5


// from-string get shadow password entry
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

	spwd.sp_namp = fields[0];
	spwd.sp_pwdp = fields[1];

	if (streq(fields[2], ""))
		spwd.sp_lstchg = -1;
	else if (a2sl(&spwd.sp_lstchg, fields[2], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

	if (streq(fields[3], ""))
		spwd.sp_min = -1;
	else if (a2sl(&spwd.sp_min, fields[3], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

	if (streq(fields[4], ""))
		spwd.sp_max = -1;
	else if (a2sl(&spwd.sp_max, fields[4], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

	if (i == OFIELDS) {
		/*
		 * If there are only OFIELDS fields, this is a SVr3.2
		 * /etc/shadow formatted file.  Initialize the other
		 * field members to -1.
		 */
		spwd.sp_warn   = -1;
		spwd.sp_inact  = -1;
		spwd.sp_expire = -1;
		spwd.sp_flag   = SHADOW_SP_FLAG_UNSET;

		return &spwd;
	}

	if (streq(fields[5], ""))
		spwd.sp_warn = -1;
	else if (a2sl(&spwd.sp_warn, fields[5], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

	if (streq(fields[6], ""))
		spwd.sp_inact = -1;
	else if (a2sl(&spwd.sp_inact, fields[6], NULL, 0, 0, LONG_MAX) == -1)
		return NULL;

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
#endif
