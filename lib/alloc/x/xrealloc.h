// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_X_XREALLOC_H_
#define SHADOW_INCLUDE_LIB_ALLOC_X_XREALLOC_H_


#include "config.h"

#include "alloc/realloc.h"
#include "x.h"


#define XREALLOC(p, n, type)  X(REALLOC(p, n, type))


#endif  // include guard
