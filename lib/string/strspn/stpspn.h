// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPSPN_H_


#include "config.h"

#include <string.h>

#include "attr.h"


// string returns-pointer substring prefix length
// Similar to strspn(3), but return a pointer instead of an offset.
// Similar to strchrnul(3), but search for any bytes not in 'accept'.
#define stpspn                                                        \
((static inline auto *(auto *s, const char *accept))                  \
{                                                                     \
	return s + strspn(s, accept);                                 \
})


#endif  // include guard
