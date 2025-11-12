// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_


#include "config.h"

#include <string.h>

#include "attr.h"


// strnul - string NUL
#define strnul                                                        \
((static inline auto *(auto *s))                                      \
{                                                                     \
	return s + strlen(s);                                         \
})


#endif  // include guard
