// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_


#include "config.h"

#include <string.h>

#include "attr.h"


// string null-byte
// Similar to strlen(3), but return a pointer instead of an offset.
#define strnul                                                        \
((static inline auto *(auto *s))                                      \
{                                                                     \
	return s + strlen(s);                                         \
})


#endif  // include guard
