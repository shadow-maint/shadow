// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_H_


#include <config.h>

#include <stddef.h>

#include "atoi/a2i/a2i.h"
#include "typetraits.h"


#define str2i(T, ...)  a2i(T, __VA_ARGS__, NULL, 0, type_min(T), type_max(T))

#define str2sh(...)    str2i(short, __VA_ARGS__)
#define str2si(...)    str2i(int, __VA_ARGS__)
#define str2sl(...)    str2i(long, __VA_ARGS__)
#define str2sll(...)   str2i(long long, __VA_ARGS__)

#define str2uh(...)    str2i(unsigned short, __VA_ARGS__)
#define str2ui(...)    str2i(unsigned int, __VA_ARGS__)
#define str2ul(...)    str2i(unsigned long, __VA_ARGS__)
#define str2ull(...)   str2i(unsigned long long, __VA_ARGS__)


#endif  // include guard
