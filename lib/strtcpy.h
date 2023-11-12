/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_STRTCPY_H_
#define SHADOW_INCLUDE_LIB_STRTCPY_H_


#include <config.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "sizeof.h"


/*
 * SYNOPSIS
 *	int STRTCPY(char dst[restrict], const char *restrict src);
 *
 * ARGUMENTS
 *	dst	Destination buffer where to copy a string.
 *	src	Source string to be copied into dst.
 *
 * DESCRIPTION
 *	This macro copies the string pointed to by src, into a string
 *	at the buffer pointed to by dst.  If the destination buffer,
 *	isn't large enough to hold the copy, the resulting string is
 *	truncated.  The size of the buffer is calculated internally via
 *	SIZEOF_ARRAY().
 *
 * RETURN VALUE
 *	-1	If this call truncated the resulting string.
 *
 *	strlen(dst)
 *		On success.
 *
 * ERRORS
 *	This function doesn't set errno.
 */


#define STRTCPY(dst, src)  strtcpy(dst, src, SIZEOF_ARRAY(dst))


inline ssize_t strtcpy(char *restrict dst, const char *restrict src,
    size_t dsize);


inline ssize_t
strtcpy(char *restrict dst, const char *restrict src, size_t dsize)
{
	bool    trunc;
	char    *p;
	size_t  dlen, slen;

	if (dsize == 0)
		return -1;

	slen = strnlen(src, dsize);
	trunc = (slen == dsize);
	dlen = slen - trunc;

	p = mempcpy(dst, src, dlen);
	*p = '\0';

	return trunc ? -1 : slen;
}


#endif  // include guard
