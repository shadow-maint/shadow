// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_


#include "config.h"

#include <ctype.h>
#include <stdbool.h>

#include "string/strcmp/streq.h"


inline bool strchriscntrl(const char *s);


// string character is [:cntrl:]
// Return true if any control character is found in the string.
// Check both C0 (0x00-0x1F, 0x7F) via iscntrl(3) and C1 (0x80-0x9F)
// explicitly, since glibc's iscntrl() does not classify C1 bytes as
// control characters in any locale.
inline bool
strchriscntrl(const char *s)
{
	for (; !streq(s, ""); s++) {
		unsigned char  c = *s;

		if (iscntrl(c))
			return true;
		if (c >= 0x80 && c <= 0x9F)
			return true;
	}

	return false;
}


#endif  // include guard
