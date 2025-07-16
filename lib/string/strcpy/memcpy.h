// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMCPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMCPY_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"

#include <assert.h>


// memcpy_a - memory copy array
#define memcpy_a(dst, src)                                            \
({                                                                    \
	static_assert(sizeof_a(dst) == sizeof_a(src), "");            \
                                                                      \
	memcpy(dst, src, sizeof_a(dst));                              \
})


#endif  // include guard
