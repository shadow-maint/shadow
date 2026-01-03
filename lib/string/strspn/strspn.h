// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STRSPN_H_


#include "config.h"

#include <string.h>

#include "attr.h"


// stpspn - string offset-pointer span
#define stpspn(s, accept)                                             \
({                                                                    \
	__auto_type  s_ = s;                                          \
	                                                              \
	s_ + strspn(s_, accept);                                      \
})


#endif  // include guard
