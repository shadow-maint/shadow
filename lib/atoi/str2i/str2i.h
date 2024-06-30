// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_STR2I_STR2I_H_


#include <config.h>

#include "atoi/str2i/str2s.h"
#include "atoi/str2i/str2u.h"


#define str2i(TYPE, ...)                                                      \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		short:              str2sh,                                   \
		int:                str2si,                                   \
		long:               str2sl,                                   \
		long long:          str2sll,                                  \
		unsigned short:     str2uh,                                   \
		unsigned int:       str2ui,                                   \
		unsigned long:      str2ul,                                   \
		unsigned long long: str2ull                                   \
	)(__VA_ARGS__)                                                        \
)


#endif  // include guard
