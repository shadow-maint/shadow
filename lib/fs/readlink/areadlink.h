// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_READLINK_AREADLINK_H_
#define SHADOW_INCLUDE_LIB_FS_READLINK_AREADLINK_H_


#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "alloc/malloc.h"
#include "attr.h"
#include "fs/readlink/readlinknul.h"


ATTR_STRING(1)
inline char *areadlink(const char *path);


// Similar to readlink(2), but allocate and terminate the string.
inline char *
areadlink(const char *filename)
{
	size_t size = 1024;

	while (true) {
		int  len;
		char *buffer = MALLOC(size, char);
		if (NULL == buffer) {
			return NULL;
		}

		len = readlinknul(filename, buffer, size);
		if (len != -1)
			return buffer;

		free(buffer);
		if (errno != E2BIG)
			return NULL;

		size *= 2;
	}
}


#endif  // include guard
