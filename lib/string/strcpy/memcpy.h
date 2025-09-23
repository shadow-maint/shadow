// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMCPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_MEMCPY_H_


#include "config.h"

#include <string.h>

#include "sizeof.h"

#include <assert.h>


#define MEMCPY(dst, src)                                              \
({                                                                    \
	static_assert(SIZEOF_ARRAY(dst) == SIZEOF_ARRAY(src), "");    \
                                                                      \
	memcpy(dst, src, SIZEOF_ARRAY(dst));                          \
})


#endif  // include guard
