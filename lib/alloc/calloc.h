// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_CALLOC_H_


#include "config.h"

#include <stdlib.h>

#include "exit_if_null.h"


#define CALLOC(n, type)                                                       \
(                                                                             \
	(type *) calloc(n, sizeof(type))                                      \
)


#define XCALLOC(n, type)  exit_if_null(CALLOC(n, type))


#endif  // include guard
