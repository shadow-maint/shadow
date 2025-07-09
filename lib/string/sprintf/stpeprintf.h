// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPEPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPEPRINTF_H_


#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "attr.h"


#if !defined(HAVE_STPEPRINTF)
format_attr(printf, 3, 4)
inline char *stpeprintf(char *dst, const char *end,
    const char *restrict fmt, ...);
format_attr(printf, 3, 0)
inline char *vstpeprintf(char *dst, const char *end,
    const char *restrict fmt, va_list ap);
#endif


/*
 * SYNOPSIS
 *	[[gnu::format(printf, 3, 4)]]
 *	char *_Nullable stpeprintf(char *_Nullable dst, const char end[0],
 *	                           const char *restrict fmt, ...);
 *
 *	[[gnu::format(printf, 3, 0)]]
 *	char *_Nullable vstpeprintf(char *_Nullable dst, const char end[0],
 *	                           const char *restrict fmt, va_list ap);
 *
 *
 * ARGUMENTS
 *	dst	Destination buffer where to write a string.
 *
 *	end	Pointer to one after the last element of the buffer
 *		pointed to by `dst`.  Usually, it should be calculated
 *		as `dst + countof(dst)`.
 *
 *	fmt	Format string
 *
 *	...
 *	ap	Variadic argument list
 *
 * DESCRIPTION
 *	These functions are very similar to [v]snprintf(3).
 *
 *	The destination buffer is limited by a pointer to its end --one
 *	after its last element-- instead of a size.  This allows
 *	chaining calls to it safely, unlike [v]snprintf(3), which is
 *	difficult to chain without invoking Undefined Behavior.
 *
 * RETURN VALUE
 *	dst + strlen(dst)
 *		•  On success, these functions return a pointer to the
 *		   terminating NUL byte.
 *
 *	NULL
 *		•  If this call truncated the resulting string (E2BIG).
 *		•  If this function failed (see ERRORS).
 *		•  If `dst == NULL` (a previous chained call to these
 *		   functions failed).
 *
 * ERRORS
 *	These functions may fail for the same reasons as vsnprintf(3).
 *
 *	E2BIG
 *		•  If this call truncated the resulting string.
 */


#if !defined(HAVE_STPEPRINTF)
inline char *
stpeprintf(char *dst, const char *end, const char *restrict fmt, ...)
{
	char     *p;
	va_list  ap;

	va_start(ap, fmt);
	p = vstpeprintf(dst, end, fmt, ap);
	va_end(ap);

	return p;
}
#endif


#if !defined(HAVE_STPEPRINTF)
inline char *
vstpeprintf(char *dst, const char *end, const char *restrict fmt, va_list ap)
{
	int        len;
	ptrdiff_t  size;

	if (dst == NULL)
		return NULL;

	size = end - dst;
	len = vsnprintf(dst, size, fmt, ap);

	if (len == -1)
		return NULL;
	if (len >= size) {
		errno = E2BIG;
		return NULL;
	}

	return dst + len;
}
#endif


#endif  // include guard
