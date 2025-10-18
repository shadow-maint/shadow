// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRDUP_STRNDUP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRDUP_STRNDUP_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"


#define STRNDUP(strn)  strndup(strn, countof(strn))


#endif  // include guard
