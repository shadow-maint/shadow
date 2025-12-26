// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_MEMDUP_H_


#include "config.h"

#include <stddef.h>

#include "alloc/malloc.h"


#define MEMDUP(p_, T)                                                 \
((static inline typeof(T) *(const typeof(T) *p))                      \
{                                                                     \
	typeof(T)  *new;                                              \
                                                                      \
	new = MALLOC(1, T);                                           \
	if (new == NULL)                                              \
		return NULL;                                          \
                                                                      \
	*new = *p;                                                    \
	return new;                                                   \
}(p_))


#endif  // include guard
