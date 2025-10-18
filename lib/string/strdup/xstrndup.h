// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRNDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_XSTRNDUP_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"
#include "x.h"


// Similar to strndup(3), but ensure that 's' is an array.
#define STRNDUP(s)   strndup(s, countof(s))

// exit-on-error nonstring duplicate-into-string
#define XSTRNDUP(s)  exit_if_null(STRNDUP(s))


#endif  // include guard
