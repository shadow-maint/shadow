// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"
#include "sizeof.h"


#define REALLOC(p, n, T)   REALLOC_(p, n, typeas(T))
#define REALLOC_(p, n, T)                                             \
({                                                                    \
	_Generic(p, T *: (void)0);                                    \
	(T *){reallocarray_(p, n, sizeof(T))};                        \
})

#define reallocarray_(p, n, size)  reallocarray(p, (n) ?: 1, (size) ?: 1)


#define XREALLOC(p, n, T)  exit_if_null(REALLOC(p, n, T))


#endif  // include guard
