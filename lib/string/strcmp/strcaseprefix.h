// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEPREFIX_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEPREFIX_H_


#include "config.h"

#include <stddef.h>
#include <string.h>

#include "attr.h"


// strcaseprefix - string case-insensitive prefix
#define strcaseprefix                                                 \
((static inline auto *                                                \
  (auto *s, const char *prefix)                                       \
  ATTR_STRING(1) ATTR_STRING(2))                                      \
{                                                                     \
	if (strncasecmp(s, prefix, strlen(prefix)) != 0)              \
		return NULL;                                          \
                                                                      \
	return s + strlen(prefix);                                    \
})


#endif  // include guard
