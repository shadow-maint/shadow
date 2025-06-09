// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_STPRINTF_H_


#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "attr.h"
#include "sizeof.h"


#define STPRINTF(s, fmt, ...)                                         \
(                                                                     \
	stprintf(s, countof(s), fmt __VA_OPT__(,) __VA_ARGS__)        \
)


format_attr(printf, 3, 4)
inline int stprintf(char *restrict s, int size, const char *restrict fmt, ...);
format_attr(printf, 3, 0)
inline int vstprintf(char *restrict s, int size, const char *restrict fmt,
    va_list ap);


// string truncate print formatted
inline int
stprintf(char *restrict s, int size, const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = vstprintf(s, size, fmt, ap);
	va_end(ap);

	return len;
}


inline int
vstprintf(char *restrict s, int size, const char *restrict fmt, va_list ap)
{
	int  len;

	len = vsnprintf(s, size, fmt, ap);
	if (len == -1)
		return -1;
	if (len >= size) {
		errno = E2BIG;
		return -1;
	}

	return len;
}


#endif  // include guard
