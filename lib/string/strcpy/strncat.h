// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCAT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRNCAT_H_


#include <config.h>

#include <string.h>

#include "sizeof.h"


#define STRNCAT(dst, src)  strncat(dst, src, countof(src))


#endif  // include guard
