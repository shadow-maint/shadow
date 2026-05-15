// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_SEPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_SEPRINTF_H_


#include "config.h"

#include <stdarg.h>
#include <stddef.h>

#include "attr.h"
#include "string/sprintf/stprintf.h"


#if !defined(HAVE_SEPRINTF)
// seprintf - string end-pointer print formatted
format_attr(printf, 3, 4)
inline char *seprintf(char *dst, const char end[0], const char *restrict fmt,
    ...);
// vseprintf - va_list string end-pointer print formatted
format_attr(printf, 3, 0)
inline char *vseprintf(char *dst, const char end[0], const char *restrict fmt,
    va_list ap);
#endif


#if !defined(HAVE_SEPRINTF)
inline char *
seprintf(char *dst, const char end[0], const char *restrict fmt, ...)
{
	char     *p;
	va_list  ap;

	va_start(ap, fmt);
	p = vseprintf(dst, end, fmt, ap);
	va_end(ap);

	return p;
}
#endif


#if !defined(HAVE_SEPRINTF)
inline char *
vseprintf(char *dst, const char end[0], const char *restrict fmt, va_list ap)
{
	int        len;
	ptrdiff_t  size;

	if (dst == NULL)
		return NULL;

	size = end - dst;
	len = vstprintf(dst, size, fmt, ap);
	if (len == -1)
		return NULL;

	return dst + len;
}
#endif


#endif  // include guard
