// SPDX-FileCopyrightText: 2022-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_


#include "config.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "attr.h"


#if !defined(HAVE_STPECPY)
ATTR_STRING(3)
inline char *stpecpy(char *p, const char *end, const char *restrict src);
#endif


/*
 * SYNOPSIS
 *	[[gnu::null_terminated_string_arg(3)]]
 *	char *_Nullable stpecpy(char *_Nullable p, const char end[0],
 *	                        const char *restrict src);
 *
 * ARGUMENTS
 *	p	Destination buffer where to copy a string.
 *
 *	end	Pointer to one after the last element of the buffer
 *		pointed to by `p`.  Usually, it should be calculated
 *		as `p + countof(p)`.
 *
 *	src	Source string to be copied into p.
 *
 * DESCRIPTION
 *	This function copies the string pointed to by src, into a string
 *	at the buffer pointed to by p.  If the destination buffer,
 *	limited by a pointer to its end --one after its last element--,
 *	isn't large enough to hold the copy, the resulting string is
 *	truncated.
 *
 *	This function can be chained with calls to [v]stpeprintf().
 *
 * RETURN VALUE
 *	p + strlen(p)
 *		•  On success, this function returns a pointer to the
 *		   terminating NUL byte.
 *
 *	NULL
 *		•  If this call truncated the resulting string.
 *		•  If `p == NULL` (a previous chained call to
 *		   [v]stpeprintf() failed or truncated).
 *
 * ERRORS
 *	E2BIG
 *		•  If this call truncated the resulting string.
 */


#if !defined(HAVE_STPECPY)
inline char *
stpecpy(char *p, const char *end, const char *restrict src)
{
	bool    trunc;
	size_t  dsize, dlen, slen;

	if (p == NULL)
		return NULL;

	dsize = end - p;
	slen = strnlen(src, dsize);
	trunc = (slen == dsize);
	dlen = slen - trunc;

	p = stpcpy(mempcpy(p, src, dlen), "");

	if (trunc) {
		errno = E2BIG;
		return NULL;
	}
	return p;
}
#endif


#endif  // include guard
