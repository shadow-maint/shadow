// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_XASPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_XASPRINTF_H_


#include <config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "attr.h"
#include "string/sprintf/aprintf.h"


ATTR_MALLOC(free)
format_attr(printf, 1, 2)
inline char *xaprintf(const char *restrict fmt, ...);

ATTR_MALLOC(free)
format_attr(printf, 1, 0)
inline char *xvaprintf(const char *restrict fmt, va_list ap);


// exit-on-error allocate print formatted
inline char *
xaprintf(const char *restrict fmt, ...)
{
	char     *p;
	va_list  ap;

	va_start(ap, fmt);
	p = xvaprintf(fmt, ap);
	va_end(ap);

	return p;
}


inline char *
xvaprintf(const char *restrict fmt, va_list ap)
{
	char  *p;

	p = vaprintf(fmt, ap);
	if (p == NULL) {
		perror("vaprintf");
		exit(EXIT_FAILURE);
	}

	return p;
}


#endif  // include guard
