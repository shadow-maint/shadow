// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_STRNDUP_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_STRNDUP_H_


#include "config.h"

#include <memory.h>

#include "sizeof.h"
#include "exit_if_null.h"


// strndup_a - nonstring duplicate-into-string array
#define strndup_a(s)   strndup(s, countof(s))

// xstrndup_a - exit-on-error nonstring duplicate-into-string array
#define xstrndup_a(s)  exit_if_null(strndup_a(s))


#endif  // include guard
