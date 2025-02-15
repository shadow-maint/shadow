// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_


#include <config.h>

#include <string.h>

#include "attr.h"


// string null-byte
// Similar to strlen(3), but return a pointer instead of an offset.
#define strnul(s)                                                             \
({                                                                            \
	__auto_type  s_ = s;                                                  \
                                                                              \
	s_ + strlen(s_);                                                      \
})


#endif  // include guard
