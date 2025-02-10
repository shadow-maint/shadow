// SPDX-FileCopyrightText: 2022-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPEPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPEPRINTF_H_


#include "config.h"

#include <stdarg.h>
#include <stddef.h>

#include "attr.h"
#include "string/sprintf/snprintf.h"


#if !defined(HAVE_STPEPRINTF)
// stpeprintf - string offset-pointer end-pointer print formatted
format_attr(printf, 3, 4)
inline char *stpeprintf(char *dst, char *end, const char *restrict fmt, ...);
// vstpeprintf - va_list string offset-pointer end-pointer print formatted
format_attr(printf, 3, 0)
inline char *vstpeprintf(char *dst, char *end, const char *restrict fmt,
    va_list ap);
#endif


#if !defined(HAVE_STPEPRINTF)
inline char *
stpeprintf(char *dst, char *end, const char *restrict fmt, ...)
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
vstpeprintf(char *dst, char *end, const char *restrict fmt, va_list ap)
{
	int        len;
	ptrdiff_t  size;

	if (dst == NULL)
		return NULL;

	size = end - dst;
	len = vsnprintf_(dst, size, fmt, ap);
	if (len == -1)
		return NULL;

	return dst + len;
}
#endif


#endif  // include guard
