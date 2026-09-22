// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_STRNDUP_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_STRNDUP_H_


#include "config.h"

#include <memory.h>

#include "exit_if_null.h"
#include "pragma.h"
#include "sizeof.h"


// strndup_a - nonstring duplicate-into-string array
#define strndup_a(s)   DEPRECATED(strndup(s, countof(s)))

// xstrndup_a - exit-on-error nonstring duplicate-into-string array
#define xstrndup_a(s)  exit_if_null(strndup_a(s))


ATTR_DEPRECATED typeof(strndup)  strndup;


#endif  // include guard
