// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_APRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_APRINTF_H_


#include <config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "attr.h"


ATTR_MALLOC(free)
format_attr(printf, 1, 2)
inline char *aprintf(const char *restrict fmt, ...);

ATTR_MALLOC(free)
format_attr(printf, 1, 0)
inline char *vaprintf(const char *restrict fmt, va_list ap);


// allocate print formatted
// Like asprintf(3), but simpler; omit the length.
inline char *
aprintf(const char *restrict fmt, ...)
{
	char     *p;
	va_list  ap;

	va_start(ap, fmt);
	p = vaprintf(fmt, ap);
	va_end(ap);

	return p;
}


// Like vasprintf(3), but simpler; omit the length.
inline char *
vaprintf(const char *restrict fmt, va_list ap)
{
	char  *p;

	if (vasprintf(&p, fmt, ap) == -1)
		return NULL;

	return p;
}


#endif  // include guard
