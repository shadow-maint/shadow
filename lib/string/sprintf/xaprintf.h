// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_SPRINTF_XASPRINTF_H_
#define SHADOW_INCLUDE_LIB_STRING_SPRINTF_XASPRINTF_H_


#include <config.h>

#include "string/sprintf/aprintf.h"
#include "x.h"


// exit-on-error allocate print formatted
#define xaprintf(...)  X(aprintf(__VA_ARGS__))


#endif  // include guard
