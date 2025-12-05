// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_SNPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_SNPRINTF_H_


#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "attr.h"
#include "sizeof.h"


// stprintf_a - string truncate print formatted array
#define stprintf_a(s, fmt, ...)                                               \
(                                                                             \
	snprintf_(s, countof(s), fmt __VA_OPT__(,) __VA_ARGS__)               \
)


format_attr(printf, 3, 4)
inline int snprintf_(char *restrict s, ssize_t size, const char *restrict fmt, ...);
format_attr(printf, 3, 0)
inline int vsnprintf_(char *restrict s, ssize_t size, const char *restrict fmt,
    va_list ap);


inline int
snprintf_(char *restrict s, ssize_t size, const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = vsnprintf_(s, size, fmt, ap);
	va_end(ap);

	return len;
}


inline int
vsnprintf_(char *restrict s, ssize_t size, const char *restrict fmt, va_list ap)
{
	int  len;

	if (size == 0)
		abort();

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
