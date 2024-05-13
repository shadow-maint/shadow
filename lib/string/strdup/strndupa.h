// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_STRNDUPA_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_STRNDUPA_H_


#include <config.h>

#include <alloca.h>
#include <string.h>

#include "sizeof.h"
#include "string/strcpy/strncat.h"


// Similar to strndupa(3), but ensure that 's' is an array.
#define STRNDUPA(s)                                                           \
(                                                                             \
	STRNCAT(strcpy(alloca(strnlen(s, NITEMS(s)) + 1), ""), s)             \
)


#endif  // include guard
