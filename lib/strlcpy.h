/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_STRLCPY_H_
#define SHADOW_INCLUDE_LIB_STRLCPY_H_


#include <config.h>

#include <stddef.h>
#include <string.h>

#include "sizeof.h"


/*
 * SYNOPSIS
 *	int STRLCPY(char dst[restrict], const char *restrict src);
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


#define STRLCPY(dst, src)  strlcpy_(dst, src, SIZEOF_ARRAY(dst))


inline size_t strlcpy_(char *restrict dst, const char *restrict src,
    size_t size);


inline size_t
strlcpy_(char *restrict dst, const char *restrict src, size_t size)
{
	size_t  len;

	len = strlcpy(dst, src, size);

	return (len >= size) ? -1 : len;
}


#endif  // include guard
