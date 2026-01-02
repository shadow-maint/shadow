// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_STRCHRISCNTRL_H_


#include "config.h"

#include <stdbool.h>

#include "string/ctype/isascii.h"
#include "string/strcmp/streq.h"


inline bool strchriscntrl(const char *s);


// strchriscntrl - string character is control-character (C0 or C1)
inline bool
strchriscntrl(const char *s)
{
	for (; !streq(s, ""); s++) {
		unsigned char  c = *s;

		if (iscntrl_c(c))
			return true;
		if (c >= 0x80 && c <= 0x9F)
			return true;
	}

	return false;
}


#endif  // include guard
