// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRTOASCII_STRTOLOWER_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRTOASCII_STRTOLOWER_H_


#include <config.h>

#include <ctype.h>

#include "string/strcmp/streq.h"


inline char *strtolower(char *str);


// string convert-to lower-case
// Like tolower(3), but convert all characters in the string.
// Returns the input pointer.
inline char *
strtolower(char *str)
{
	for (char *s = str; !streq(s, ""); s++) {
		unsigned char  c = *s;

		*s = tolower(c);
	}
	return str;
}


#endif  // include guard
