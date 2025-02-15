// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STREQ_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STREQ_H_


#include <config.h>

#include <stdbool.h>
#include <string.h>

#include "attr.h"


ATTR_STRING(1)
ATTR_STRING(2)
inline bool streq(const char *s1, const char *s2);


// strings equal
/* Return true if s1 and s2 compare equal.  */
inline bool
streq(const char *s1, const char *s2)
{
	return strcmp(s1, s2) == 0;
}


#endif  // include guard
