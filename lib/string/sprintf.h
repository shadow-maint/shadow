/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_SPRINTF_H_
#define SHADOW_INCLUDE_LIB_SPRINTF_H_


#include <config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "attr.h"
#include "defines.h"
#include "sizeof.h"


#define SNPRINTF(s, fmt, ...)                                                 \
({                                                                            \
	size_t  sz_, len_;                                                    \
                                                                              \
	sz_ = NITEMS(s);                                                      \
	len_ = snprintf(s, sz_, fmt __VA_OPT__(,) __VA_ARGS__);               \
                                                                              \
	(len_ >= sz_) ? -1 : len_;                                            \
})


format_attr(printf, 2, 3)
inline int xasprintf(char **restrict s, const char *restrict fmt, ...);
format_attr(printf, 2, 0)
inline int xvasprintf(char **restrict s, const char *restrict fmt, va_list ap);


inline int
xasprintf(char **restrict s, const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = xvasprintf(s, fmt, ap);
	va_end(ap);

	return len;
}


inline int
xvasprintf(char **restrict s, const char *restrict fmt, va_list ap)
{
	int  len;

	len = vasprintf(s, fmt, ap);
	if (len == -1) {
		perror("asprintf");
		exit(EXIT_FAILURE);
	}

	return len;
}


#endif  // include guard
