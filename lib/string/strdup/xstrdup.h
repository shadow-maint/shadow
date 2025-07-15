// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2006, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008     , Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRDUP_H_


#include "config.h"

#include <string.h>

#include "alloc/x/xmalloc.h"
#include "attr.h"


ATTR_MALLOC(free)
inline char *xstrdup(const char *str);


inline char *
xstrdup(const char *str)
{
	return strcpy(XMALLOC(strlen(str) + 1, char), str);
}


#endif  // include guard
