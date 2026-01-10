// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"
#include "sizeof.h"


// calloc_T - calloc type-safe
#define calloc_T(n, T)   calloc_T_(n, typeas(T))
#define calloc_T_(n, T)                                               \
(                                                                     \
	(void)0,                                                      \
	(T *){calloc(n, sizeof(T))}                                   \
)


// xcalloc_T - exit-on-error calloc type-safe
#define xcalloc_T(n, T)  exit_if_null(calloc_T(n, T))


#endif  // include guard
