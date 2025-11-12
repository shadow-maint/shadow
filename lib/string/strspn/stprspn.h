// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_


#include "config.h"

#include <string.h>

#include "string/strchr/strnul.h"
#include "string/strspn/strrspn.h"


// stprspn - string returns-pointer rear span
#define stprspn                                                       \
((static inline auto *(auto *s, const char *accept))                  \
{                                                                     \
	return strnul(s) - strrspn_(s, accept);                       \
})


#endif  // include guard
