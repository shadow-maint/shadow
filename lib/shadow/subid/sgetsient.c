// SPDX-FileCopyrightText: 2012, Eric Biederman
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/subid/sgetsient.h"

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "../libsubid/subid.h"

#include "alloc/malloc.h"
#include "atoi/str2i.h"
#include "string/strcmp/streq.h"
#include "string/strtok/strsep2arr.h"


#ifdef ENABLE_SUBIDS
// sgetsient - from-string get sub-ID entry
struct subordinate_range *
sgetsient(const char *s)
{
	static struct subordinate_range range;
	static char rangebuf[1024];

	char *fields[SUBID_NFIELDS];

	/*
	 * Copy the string to a temporary buffer so the substrings can
	 * be modified to be NULL terminated.
	 */
	if (strlen(s) >= sizeof(rangebuf))
		return NULL;	/* fail if too long */
	strcpy(rangebuf, s);

	if (strsep2arr_a(rangebuf, ":", fields) == -1)
		return NULL;

	if (streq(fields[0], ""))
		return NULL;
	range.owner = fields[0];
	if (str2ul(&range.start, fields[1]) == -1)
		return NULL;
	if (str2ul(&range.count, fields[2]) == -1)
		return NULL;

	return &range;
}
#endif
