// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STPECPY_H_


#include "config.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "attr.h"
#include "string/strcpy/strtcpy.h"


#if !defined(HAVE_STPECPY)
// stpecpy - string offset-pointer end-pointer copy
ATTR_STRING(3)
inline char *stpecpy(char *dst, const char end[0], const char *restrict src);
#endif


#if !defined(HAVE_STPECPY)
inline char *
stpecpy(char *dst, const char end[0], const char *restrict src)
{
	ssize_t  dlen;

	if (dst == NULL)
		return NULL;

	dlen = strtcpy(dst, src, end - dst);
	if (dlen == -1)
		return NULL;

	return dst + dlen;
}
#endif


#endif  // include guard
