// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_


#include "config.h"

#include <stddef.h>

#include "alloc/malloc.h"


// memdup_T - memory duplicate type-safe
#define memdup_T(p, T)   memdup_T_(p, typeas(T))
#define memdup_T_(..., T)                                             \
((static inline T *(const T *p))                                      \
{                                                                     \
	T  *new;                                                      \
                                                                      \
	new = malloc_T(1, T);                                         \
	if (new == NULL)                                              \
		return NULL;                                          \
                                                                      \
	*new = *p;                                                    \
	return new;                                                   \
}(__VA_ARGS__))


#endif  // include guard
