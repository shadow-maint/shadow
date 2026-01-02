// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_H_


#include "config.h"

#include <stdbool.h>

#include "string/ctype/isascii.h"
#include "string/strcmp/streq.h"
#include "string/strspn/stpspn.h"


inline bool strisdigit(const char *s); // strisdigit - string is [:digit:]
inline bool strisprint(const char *s); // strisprint - string is [:print:]


inline bool
strisdigit(const char *s)
{
	return !streq(s, "") && streq(stpspn(s, CTYPE_DIGIT_C), "");
}
inline bool
strisprint(const char *s)
{
	return !streq(s, "") && streq(stpspn(s, CTYPE_PRINT_C), "");
}


#endif  // include guard
