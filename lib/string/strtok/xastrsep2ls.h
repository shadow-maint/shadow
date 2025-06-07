// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_XASTRSEP2LS_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_XASTRSEP2LS_H_


#include "config.h"

#include "string/strtok/astrsep2ls.h"
#include "x.h"


#define xastrsep2ls(s, delim, np)  X(astrsep2ls(s, delim, np))


#endif  // include guard
