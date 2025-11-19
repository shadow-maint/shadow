// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MACRO_H_
#define SHADOW_INCLUDE_LIB_MACRO_H_


#include "config.h"


// DEFARG - default argument
#define DEFARG(def, arg)    DEFARG_(def, arg)
#define DEFARG_(def, ...)   DEFARG__(__VA_ARGS__ __VA_OPT__(,) def)
#define DEFARG__(x, ...)    (x)


#endif  // include guard
