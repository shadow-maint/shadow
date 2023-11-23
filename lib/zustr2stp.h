/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIBMISC_ZUSTR2STP_H_
#define SHADOW_INCLUDE_LIBMISC_ZUSTR2STP_H_


#include <config.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "must_be.h"
#include "sizeof.h"


#define ZUSTR2STP(dst, src)                                                   \
({                                                                            \
	static_assert(!is_array(dst) || sizeof(dst) > SIZEOF_ARRAY(src), ""); \
                                                                              \
	zustr2stp(dst, src, NITEMS(src));                                     \
})


inline char *zustr2stp(char *restrict dst, const char *restrict src, size_t sz);


/*
 * SYNOPSIS
 *	char *zustr2stp(char *restrict dst,
 *	                const char src[restrict .sz], size_t sz);
 *
 * ARGUMENTS
 *	dst	Destination buffer where to copy a string.
 *
 *	src	Source null-padded character sequence to be copied into
 *	        dst.
 *
 *	sz	Size of the *source* buffer.
 *
 * DESCRIPTION
 *	This function copies the null-padded character sequence pointed
 *	to by src, into a string at the buffer pointed to by dst.
 *
 * RETURN VALUE
 *	dst + strlen(dst)
 *		This function returns a pointer to the terminating NUL
 *		byte.
 *
 * ERRORS
 *	This function doesn't set errno.
 *
 * CAVEATS
 *	This function doesn't know the size of the destination buffer.
 *	It assumes it will always be large enough.  Since the size of
 *	the source buffer is known to the caller, it should make sure to
 *	allocate a destination buffer of at least `sz + 1`.
 *
 * EXAMPLES
 *	char  src[13] = "Hello, world!"  // No '\0' in this buffer!
 *	char  dst[NITEMS(src) + 1];
 *
 *	zustr2stp(dst, src, NITEMS(src));
 *	puts(dst);
 */


inline char *
zustr2stp(char *restrict dst, const char *restrict src, size_t sz)
{
	char  *p;

	p = mempcpy(dst, src, strnlen(src, sz));
	*p = '\0';

	return p;
}


#endif  // include guard
