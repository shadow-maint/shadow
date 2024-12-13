// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_


#include <config.h>

#include <ctype.h>
#include <stdbool.h>

#include "string/strcmp/streq.h"


inline bool strchriscntrl(const char *s);


// string character is [:cntrl:]
// Return true if any iscntrl(3) character is found in the string.
inline bool
strchriscntrl(const char *s)
{
	for (; !streq(s, ""); s++) {
		unsigned char  c = *s;

		if (iscntrl(c))
			return true;
	}

	return false;
}


#endif  // include guard
