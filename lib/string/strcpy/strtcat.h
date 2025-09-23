// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCAT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCAT_H_


#include "config.h"

#include <string.h>
#include <sys/types.h>

#include "attr.h"
#include "sizeof.h"
#include "string/strcpy/strtcpy.h"


#define STRTCAT(dst, src)  strtcat(dst, src, countof(dst))


ATTR_STRING(2)
inline ssize_t strtcat(char *restrict dst, const char *restrict src,
    ssize_t dsize);


// string truncate catenate
// Returns new length, or -1 on truncation.
inline ssize_t
strtcat(char *restrict dst, const char *restrict src, ssize_t dsize)
{
	ssize_t  oldlen, n;

	oldlen = strlen(dst);

	n = strtcpy(dst + oldlen, src, dsize - oldlen);
	if (n == -1)
		return -1;

	return oldlen + n;
}


#endif  // include guard
