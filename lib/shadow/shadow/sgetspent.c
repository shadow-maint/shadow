// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/shadow/sgetspent.h"

#include <errno.h>
#include <shadow.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "atoi/a2i.h"
#include "alloc/malloc.h"
#include "defines.h"
#include "prototypes.h"
#include "shadowlog_internal.h"
#include "sizeof.h"
#include "string/strcmp/streq.h"
#include "string/strcpy/strtcpy.h"
#include "string/strtok/stpsep.h"
#include "string/strtok/strsep2arr.h"


#define	FIELDS	9
#define	OFIELDS	5


#ifndef HAVE_SGETSPENT
// from-string get shadow password entry
struct spwd *
sgetspent(const char *s)
{
	static char         *buf = NULL;
	static struct spwd  spent = {};

	int          e;
	size_t       size;
	struct spwd  *dummy;

	size = strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	e = sgetspent_r(s, &spent, buf, size, &dummy);
	if (e != 0) {
		errno = e;
		return NULL;
	}

	return &spent;
}
#endif


#ifndef HAVE_SGETSPENT_R
// from-string get shadow password entry re-entrant
int
sgetspent_r(size_t size;
    const char *restrict s, struct spwd *spent, char buf[size], size_t size,
    struct spwd **dummy)
{
	char    *fields[FIELDS];
	size_t  i;

	// 'dummy' exists only for historic reasons.
	if (dummy != NULL)
		*dummy = spent;

	if (strtcpy(buf, s, size) == -1)
		return errno;

	stpsep(buf, "\n");

	i = strsep2arr(buf, ":", countof(fields), fields);
	if (i == countof(fields) - 1)
		fields[i++] = "";
	if (i != countof(fields) && i != OFIELDS)
		return EINVAL;

	spent->sp_namp = fields[0];
	spent->sp_pwdp = fields[1];

	if (streq(fields[2], ""))
		spent->sp_lstchg = -1;
	else if (a2sl(&spent->sp_lstchg, fields[2], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	if (streq(fields[3], ""))
		spent->sp_min = -1;
	else if (a2sl(&spent->sp_min, fields[3], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	if (streq(fields[4], ""))
		spent->sp_max = -1;
	else if (a2sl(&spent->sp_max, fields[4], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	if (i == OFIELDS) {
		/*
		 * If there are only OFIELDS fields, this is a SVr3.2
		 * /etc/shadow formatted file.  Initialize the other
		 * field members to -1.
		 */
		spent->sp_warn   = -1;
		spent->sp_inact  = -1;
		spent->sp_expire = -1;
		spent->sp_flag   = SHADOW_SP_FLAG_UNSET;

		return 0;
	}

	if (streq(fields[5], ""))
		spent->sp_warn = -1;
	else if (a2sl(&spent->sp_warn, fields[5], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	if (streq(fields[6], ""))
		spent->sp_inact = -1;
	else if (a2sl(&spent->sp_inact, fields[6], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	if (streq(fields[7], ""))
		spent->sp_expire = -1;
	else if (a2sl(&spent->sp_expire, fields[7], NULL, 0, 0, LONG_MAX) == -1)
		return errno;

	/*
	 * This field is reserved for future use.  But it isn't supposed
	 * to have anything other than a valid integer in it.
	 */
	if (streq(fields[8], ""))
		spent->sp_flag = SHADOW_SP_FLAG_UNSET;
	else if (str2ul(&spent->sp_flag, fields[8]) == -1)
		return errno;

	return 0;
}
#endif
