// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_FPRINTF_EPRINTE_H_
#define SHADOW_INCLUDE_LIB_IO_FPRINTF_EPRINTE_H_


#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "attr.h"
#include "io/fprintf/fprinte.h"


format_attr(printf, 1, 2)
inline int eprinte(const char *restrict fmt, ...);
format_attr(printf, 1, 0)
inline int veprinte(const char *restrict fmt, va_list ap);


// stderr print error
inline int
eprinte(const char *restrict fmt, ...)
{
	int      len;
	va_list  ap;

	va_start(ap, fmt);
	len = veprinte(fmt, ap);
	va_end(ap);

	return len;
}


// va_list stderr print error
inline int
veprinte(const char *restrict fmt, va_list ap)
{
	return vfprinte(stderr, fmt, ap);
}


#endif  // include guard
