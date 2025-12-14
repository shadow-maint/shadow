// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRMOVE_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRMOVE_H_


#include "config.h"

#include <string.h>

#include "attr.h"
#include "memory/memcpy/memmove.h"


ATTR_STRING(2)
inline void strmove(char *dst, char *src);


// strmove - string move
inline void
strmove(char *dst, char *src)
{
	memmove_T(dst, src, strlen(src) + 1, char);
}


#endif  // include guard
