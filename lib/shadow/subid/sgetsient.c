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
#include "string/strcpy/strtcpy.h"
#include "string/strtok/strsep2arr.h"


#ifdef ENABLE_SUBIDS
// sgetsient - from-string get sub-ID entry
struct subordinate_range *
sgetsient(const char *s)
{
	static char                      *buf = NULL;
	static struct subordinate_range  sient;

	char *fields[SUBID_NFIELDS];
	size_t  size;

	size = strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	if (strtcpy(buf, s, size) == -1)
		return NULL;

	if (strsep2arr_a(buf, ":", fields) == -1)
		return NULL;

	if (streq(fields[0], ""))
		return NULL;
	sient.owner = fields[0];
	if (str2ul(&sient.start, fields[1]) == -1)
		return NULL;
	if (str2ul(&sient.count, fields[2]) == -1)
		return NULL;

	return &sient;
}
#endif
