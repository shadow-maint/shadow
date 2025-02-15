// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEEQ_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEEQ_H_


#include <config.h>

#include <stdbool.h>
#include <strings.h>

#include "attr.h"


ATTR_STRING(1) ATTR_STRING(2)
inline bool strcaseeq(const char *s1, const char *s2);


// strings case-insensitive equal
// streq(), but case-insensitive.
inline bool
strcaseeq(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}


#endif  // include guard
