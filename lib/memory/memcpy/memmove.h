// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMCPY_MEMMOVE_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMCPY_MEMMOVE_H_


#include "config.h"

#include <memory.h>

#include "sizeof.h"


// memmove_T - memory move type-safe
#define memmove_T(dst, src, n, T)   memmove_T_(dst, src, n, typeas(T))
#define memmove_T_(dst, src, n, T)  do                                \
{                                                                     \
	_Generic(dst, T *: (void)0);                                  \
	_Generic(src, T *: (void)0);                                  \
	memmove(dst, src, (n) * sizeof(T));                           \
} while (0)


#endif  // include guard
