// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_X_XMALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_X_XMALLOC_H_


#include "config.h"

#include "alloc/malloc.h"
#include "exit_if_null.h"


#define XMALLOC(n, type)  exit_if_null(MALLOC(n, type))


#endif  // include guard
