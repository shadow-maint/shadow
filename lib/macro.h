// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MACRO_H_
#define SHADOW_INCLUDE_LIB_MACRO_H_


#include "config.h"


#define VA_IFNOT_(...)       __VA_ARGS__
#define VA_IFNOT_1(...)
#define VA_IFNOT(cond, ...)  VA_IFNOT_ ## cond (__VA_ARGS__)

#define DEFAULT_(def, ...)                                            \
(                                                                     \
	/* default:  */ VA_IFNOT(__VA_OPT__(1), def)                  \
	/* override: */ __VA_OPT__(__VA_ARGS__)                       \
)

#define DEFAULT(def, ovr)  DEFAULT_(def, ovr)


#endif  // include guard
