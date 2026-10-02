// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMCPY_STRNCPY_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMCPY_STRNCPY_H_


#include "config.h"

#include <memory.h>

#include "attr.h"
#include "pragma.h"
#include "sizeof.h"


// strncpy_a - nonstring copy array
#define strncpy_a(dst, src)  DEPRECATED(strncpy(dst, src, countof(dst)))


ATTR_DEPRECATED typeof(strncpy)  strncpy;


#endif  // include guard
