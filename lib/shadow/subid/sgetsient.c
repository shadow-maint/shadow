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
	static struct subordinate_range  sient = {};

	int     e;
	size_t  size;

	size = strlen(s) + 1;

	free(buf);
	buf = MALLOC(size, char);
	if (buf == NULL)
		return NULL;

	e = sgetsient_r(s, &sient, buf, size);
	if (e != 0) {
		errno = e;
		return NULL;
	}

	return &sient;
}


// from-string get sub-ID entry re-entrant
int
sgetsient_r(size_t size;
    const char *restrict s, struct subordinate_range *restrict sient,
    char buf[restrict size], size_t size)
{
	char  *fields[SUBID_NFIELDS];

	if (strtcpy(buf, s, size) == -1)
		return errno;

	if (STRSEP2ARR(buf, ":", fields) == -1)
		return EINVAL;

	if (streq(fields[0], ""))
		return EINVAL;
	sient->owner = fields[0];
	if (str2ul(&sient->start, fields[1]) == -1)
		return errno;
	if (str2ul(&sient->count, fields[2]) == -1)
		return errno;

	return 0;
}
#endif
