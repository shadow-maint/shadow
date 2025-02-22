// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_READLINK_AREADLINK_H_
#define SHADOW_INCLUDE_LIB_FS_READLINK_AREADLINK_H_


#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "defines.h"
#include "alloc/malloc.h"
#include "attr.h"
#include "fs/readlink/readlinknul.h"


ATTR_STRING(1)
inline char *areadlink(const char *link);


// Similar to readlink(2), but allocate and terminate the string.
inline char *
areadlink(const char *link)
{
	size_t  size = PATH_MAX;

	while (true) {
		int   len;
		char  *buf;

		buf = MALLOC(size, char);
		if (NULL == buf)
			return NULL;

		len = readlinknul(link, buf, size);
		if (len != -1)
			return buf;

		free(buf);
		if (errno != E2BIG)
			return NULL;

		size *= 2;
	}
}


#endif  // include guard
