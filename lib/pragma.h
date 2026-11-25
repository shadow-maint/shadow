// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_PRAGMA_H_
#define SHADOW_INCLUDE_LIB_PRAGMA_H_


#include "config.h"


#define DEPRECATED(e)                                                 \
({                                                                    \
	_Pragma("GCC diagnostic push")                                \
	_Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")\
	(e);                                                          \
	_Pragma("GCC diagnostic pop")                                 \
})


#endif  // include guard
