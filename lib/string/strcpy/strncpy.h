// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCPY_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"


// strncpy_a - nonstring copy array
#define strncpy_a(dst, src)  strncpy(dst, src, countof(dst))


#endif  // include guard
