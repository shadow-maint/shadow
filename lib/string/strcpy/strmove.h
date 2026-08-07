// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRMOVE_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRMOVE_H_


#include "config.h"

#include <string.h>

#include "attr.h"
#include "string/strcpy/memmove.h"


ATTR_STRING(2)
inline char *strmove(char *dst, char *src);


// strmove - string move
inline char *
strmove(char *dst, char *src)
{
	return memmove_T(dst, src, strlen(src) + 1, char);
}


#endif  // include guard
