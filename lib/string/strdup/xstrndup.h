// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRNDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRNDUP_H_


#include <config.h>

#include <string.h>

#include "alloc/x/xmalloc.h"
#include "sizeof.h"
#include "string/strcpy/strncat.h"


// Similar to strndup(3), but ensure that 's' is an array, and exit on ENOMEM.
#define XSTRNDUP(s)                                                           \
(                                                                             \
	STRNCAT(strcpy(XMALLOC(strnlen(s, countof(s)) + 1, char), ""), s)     \
)


#endif  // include guard
