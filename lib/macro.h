// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MACRO_H_
#define SHADOW_INCLUDE_LIB_MACRO_H_


#include "config.h"


#define DEFAULT__(x, ...)    (x)
#define DEFAULT_(def, ...)   DEFAULT__(__VA_ARGS__ __VA_OPT__(,) def)
#define DEFAULT(def, arg)    DEFAULT_(def, arg)


#endif  // include guard
