// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STRNEQ_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STRNEQ_H_


#include "config.h"

#include <stdbool.h>
#include <string.h>

#include "attr.h"
#include "sizeof.h"


#define STRNEQ(strn, s)  strneq(strn, s, countof(strn))


ATTR_STRING(2)
inline bool strneq(ATTR_NONSTRING const char *strn, const char *s, size_t size);


// nonstring equal
/* Return true if the nonstring strn and the string s compare equal.  */
inline bool
strneq(const char *strn, const char *s, size_t size)
{
	if (strlen(s) > size)
		return false;

	return strncmp(strn, s, size) == 0;
}


#endif  // include guard
