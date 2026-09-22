// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_IO_FGETS_FGETS_H_
#define SHADOW_INCLUDE_LIB_IO_FGETS_FGETS_H_


#include "config.h"

#include <stdio.h>

#include "attr.h"
#include "pragma.h"
#include "sizeof.h"


// fgets_a - FILE get string array-safe
#define fgets_a(buf, stream)  DEPRECATED(fgets(buf, countof(buf), stream))


ATTR_DEPRECATED typeof(fgets)  fgets;


#endif  // include guard
