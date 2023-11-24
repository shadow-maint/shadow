/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_STRING_USTR2STP_H_
#define SHADOW_INCLUDE_LIB_STRING_USTR2STP_H_


#include <config.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "must_be.h"
#include "sizeof.h"


inline char *ustr2stp(char *restrict dst, const char *restrict src,
    size_t ssize);


/*
 * SYNOPSIS
 *	char *ustr2stp(char *restrict dst,
 *	               const char src[restrict .ssize], size_t ssize);
 *
 * ARGUMENTS
 *	dst	Destination buffer.
 *	src	Source character sequence.
 *	ssize	Size of the source character sequence.
 *
 * DESCRIPTION
 *	This function copies 'ssize' bytes from src to dst, and places a
 *	terminating null byte after that.  It reads a character
 *	sequence, and produces a string.
 *
 * RETURN VALUE
 *	dst + ssize
 *		This function returns a pointer to the terminating null
 *		byte.
 *
 * CAVEATS
 *	This function doesn't know the size of the destination buffer.
 *	It assumes it will always be large enough.  Since the size of
 *	the source buffer is known to the caller, it should make sure to
 *	allocate a destination buffer of at least `ssize + 1`.
 */


inline char *
ustr2stp(char *restrict dst, const char *restrict src, size_t ssize)
{
	char  *p;

	p = mempcpy(dst, src, ssize);
	*p = '\0';

	return p;
}


#endif  // include guard
