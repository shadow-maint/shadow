// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_STRISDIGIT_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_STRISDIGIT_H_


#include <config.h>

#include <stdbool.h>

#include "string/strcmp/streq.h"
#include "string/strspn/stpspn.h"


inline bool strisdigit(const char *s);


// string is [:digit:]
// Like isdigit(3), but check all characters in the string.
inline bool
strisdigit(const char *s)
{
	if (streq(s, ""))
		return false;

	return streq(stpspn(s, "0123456789"), "");
}


#endif  // include guard
