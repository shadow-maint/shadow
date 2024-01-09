// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETNUM_H_


#include <config.h>

#include "atoi/getlong.h"


#define getnum(TYPE, ...)                                                     \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		long:               getlong,                                  \
		long long:          getllong,                                 \
		unsigned long:      getulong,                                 \
		unsigned long long: getullong                                 \
	)(__VA_ARGS__)                                                        \
)


#define getn(TYPE, ...)                                                       \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		long:               getl,                                     \
		long long:          getll,                                    \
		unsigned long:      getul,                                    \
		unsigned long long: getull                                    \
	)(__VA_ARGS__)                                                        \
)


#endif  // include guard
