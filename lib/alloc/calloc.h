// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"


#define CALLOC(n, T)                                                  \
(                                                                     \
	(T *) calloc(n, sizeof(T))                                    \
)


#define XCALLOC(n, T)  exit_if_null(CALLOC(n, T))


#endif  // include guard
