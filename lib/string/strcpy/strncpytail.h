// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCPYTAIL_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCPYTAIL_H_


#include "config.h"

#include <stddef.h>
#include <string.h>
#include <sys/param.h>

#include "attr.h"
#include "sizeof.h"
#include "string/strchr/strnul.h"


// strncpytail_a - nonstring copy tail-of-string array
#define strncpytail_a(dst, src)  strncpytail(dst, src, countof(dst))


ATTR_STRING(2)
inline char *strncpytail(char *restrict dst, const char *restrict src,
    size_t dsize);


// strncpytail - nonstring copy tail-of-string
inline char *
strncpytail(char *restrict dst, const char *restrict src, size_t dsize)
{
	return strncpy(dst, strnul(src) - MIN(strlen(src), dsize), dsize);
}


#endif  // include guard
