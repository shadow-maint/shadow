// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPSPN_H_


#include "config.h"

#include <string.h>

#include "attr.h"


// stpspn - string offset-pointer substring prefix length
#define stpspn                                                        \
((static inline auto *(auto *s, const char *accept))                  \
{                                                                     \
	return s + strspn(s, accept);                                 \
})


#endif  // include guard
