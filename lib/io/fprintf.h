// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_FPRINTF_H_
#define SHADOW_INCLUDE_LIB_IO_FPRINTF_H_


#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "attr.h"


// eprintf - stderr print formatted
#define eprintf(...)  fprintf(stderr, __VA_ARGS__)


// fprinte - FILE* print errno
#define fprinte(stream, ...)  do                                      \
{                                                                     \
	int  e__;                                                     \
	                                                              \
	e__ = errno;                                                  \
	fprintec(stream, e__, __VA_ARGS__);                           \
	errno = e__;                                                  \
} while (0)


format_attr(printf, 3, 4)
inline int fprintec(FILE *restrict stream, int e,
    const char *restrict fmt, ...);
format_attr(printf, 3, 0)
inline int vfprintec(FILE *restrict stream, int e,
    const char *restrict fmt, va_list ap);


// fprintec - FILE* print errno code
inline int
fprintec(FILE *restrict stream, int e, const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = vfprintec(stream, e, fmt, ap);
	va_end(ap);

	return len;
}


// vfprintec - va_list FILE* print errno code
inline int
vfprintec(FILE *restrict stream, int e, const char *restrict fmt, va_list ap)
{
	int  l1, l2, l3;

	l1 = vfprintf(stream, fmt, ap);
	if (l1 < 0)
		return l1;

	l2 = fprintf(stream, l1 ? ": " : "");
	if (l2 < 0)
		return l2;

	l3 = fprintf(stream, "%s\n", strerror(e));
	if (l3 < 0)
		return l3;

	return l1 + l2 + l3;
}


#endif  // include guard
