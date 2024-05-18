// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STPCSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STPCSPN_H_


#include <config.h>

#include <string.h>

#include "attr.h"


// Similar to strcspn(3), but return a pointer instead of an offset.
// Similar to strchrnul(3), but search for several delimiters.
// Similar to strpbrk(3), but return 's + strlen(s)' if not found.
#define stpcspn(s, reject)                                                    \
({                                                                            \
	__auto_type  s_ = s;                                                  \
                                                                              \
	s_ + strcspn(s_, reject);                                             \
})


#endif  // include guard
