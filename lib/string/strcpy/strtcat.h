// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCAT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCAT_H_


#include "config.h"

#include <stddef.h>

#include "attr.h"
#include "sizeof.h"
#include "string/strchr/strnul.h"
#include "string/strcpy/stpecpy.h"


// strtcat_a - string truncate catenate array
#define strtcat_a(dst, src)  strtcat(dst, src, countof(dst))


ATTR_STRING(2)
inline ssize_t strtcat(char *restrict dst, const char *restrict src,
    size_t dsize);


// strtcat - string truncate catenate
// Returns new length, or -1 on truncation.
inline ssize_t
strtcat(char *restrict dst, const char *restrict src, size_t dsize)
{
	char  *p, *end;

	end = dst + dsize;

	p = stpecpy(strnul(dst), end, src);
	if (p == NULL)
		return -1;

	return p - dst;
}


#endif  // include guard
