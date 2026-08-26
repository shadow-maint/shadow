// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMDUP_MEMDUP_H_


#include "config.h"

#include <stddef.h>

#include "alloc/malloc.h"


// memdup_T - memory duplicate type-safe
#define memdup_T(p, T)   memdup_T_(p, typeas(T))
#define memdup_T_(p, T)                                               \
({                                                                    \
	T  *new_;                                                     \
                                                                      \
	new_ = malloc_T(1, T);                                        \
	if (new_ != NULL)                                             \
		*new_ = *(p);                                         \
	new_;                                                         \
})


#endif  // include guard
