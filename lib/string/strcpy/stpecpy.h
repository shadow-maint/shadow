// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_


#include <config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "attr.h"


ATTR_STRING(3)
inline char *stpecpy(char *dst, const char end[0], const char *restrict src);


// Like stpcpy(3), but use an 'end' pointer to truncate.
// Report truncation with NULL and E2BIG.
// Transparently pass through a NULL input.
// Calls to this function can be chained with calls to [v]seprintf().
inline char *
stpecpy(char *dst, const char end[0], const char *restrict src)
{
	bool    trunc;
	char    *p;
	size_t  dsize, dlen, slen;

	if (dst == NULL)
		return NULL;

	dsize = end - dst;
	slen = strnlen(src, dsize);
	trunc = (slen == dsize);
	dlen = slen - trunc;

	p = stpcpy(mempcpy(dst, src, dlen), "");
	if (trunc) {
		errno = E2BIG;
		return NULL;
	}

	return p;
}


#endif  // include guard
