// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRLEN_STRNLEN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRLEN_STRNLEN_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"


// strnlen_a - nonstring length array
#define strnlen_a(strn)  strnlen(strn, countof(strn))


#endif  // include guard
