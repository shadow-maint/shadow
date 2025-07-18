// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_FPRINTF_EPRINTF_H_
#define SHADOW_INCLUDE_LIB_IO_FPRINTF_EPRINTF_H_


#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "attr.h"


format_attr(printf, 1, 2)
inline int eprintf(const char *restrict fmt, ...);
format_attr(printf, 1, 0)
inline int veprintf(const char *restrict fmt, va_list ap);


inline int
eprintf(const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = veprintf(fmt, ap);
	va_end(ap);

	return len;
}


inline int
veprintf(const char *restrict fmt, va_list ap)
{
	return vfprintf(stderr, fmt, ap);
}


#endif  // include guard
