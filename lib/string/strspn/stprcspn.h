// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_


#include "config.h"

#include <string.h>

#include "string/strspn/strrcspn.h"


// stprcspn - string offset-pointer rear complement substring prefix length
#define stprcspn                                                      \
((static inline auto *(auto *s, const char *reject))                  \
{                                                                     \
	return s + strrcspn(s, reject);                               \
})


#endif  // include guard
