// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"
#include "sizeof.h"


#define CALLOC(n, T)   CALLOC_(typeas(T), n)
#define CALLOC_(T, ...)                                               \
((static inline T *(size_t n))                                        \
{                                                                     \
	return calloc(n, sizeof(T));                                  \
}(__VA_ARGS__)


#define XCALLOC(n, T)  exit_if_null(CALLOC(n, T))


#endif  // include guard
