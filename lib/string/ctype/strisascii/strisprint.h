// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_STRISPRINT_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_STRISPRINT_H_


#include "config.h"

#include <stdbool.h>

#include "string/ctype/isascii.h"
#include "string/strcmp/streq.h"


inline bool strisprint(const char *s);


// strisprint - string is [:print:]
inline bool
strisprint(const char *s)
{
	if (streq(s, ""))
		return false;

	for (; !streq(s, ""); s++) {
		if (!isprint_c(*s))
			return false;
	}

	return true;
}


#endif  // include guard
