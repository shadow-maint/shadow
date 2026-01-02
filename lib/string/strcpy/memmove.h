// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMMOVE_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMMOVE_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"


// memmove_T - memory move type-safe
#define memmove_T(dst, src, n, T)   memmove_T_(dst, src, n, typeas(T))
#define memmove_T_(dst, src, n, T)                                    \
({                                                                    \
	_Generic(dst, T *: (void)0);                                  \
	_Generic(src, T *: (void)0);                                  \
	(T *){memmove(dst, src, (n) * sizeof(T))};                    \
})


#endif  // include guard
