// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_FPRINTF_FPRINTE_H_
#define SHADOW_INCLUDE_LIB_IO_FPRINTF_FPRINTE_H_


#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "attr.h"


format_attr(printf, 2, 3)
inline int fprinte(FILE *restrict stream, const char *restrict fmt, ...);
format_attr(printf, 2, 0)
inline int vfprinte(FILE *restrict stream, const char *restrict fmt, va_list ap);


// FILE print error
inline int
fprinte(FILE *restrict stream, const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = vfprinte(stream, fmt, ap);
	va_end(ap);

	return len;
}


// va_list FILE print error
inline int
vfprinte(FILE *restrict stream, const char *restrict fmt, va_list ap)
{
	int  l1, l2, l3, e;

	e = errno;

	l1 = vfprintf(stream, fmt, ap);
	if (l1 == -1)
		goto fail;

	l2 = fprintf(stream, l1 ? ": " : "");
	if (l2 == -1)
		goto fail;

	l3 = fprintf(stream, "%s\n", strerror(e));
	if (l3 == -1)
		goto fail;

	errno = e;
	return l1 + l2 + l3;
fail:
	errno = e;
	return -1;
}


#endif  // include guard
