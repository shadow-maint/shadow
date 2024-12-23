// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_CTYPE_ISPFCHAR_H_
#define SHADOW_INCLUDE_LIB_CTYPE_ISPFCHAR_H_


#include <config.h>

#include <ctype.h>
#include <stdbool.h>


inline bool ispfchar(unsigned char c);


// is portable filename character
// Return true if 'c' is a character from the Portable Filename Character Set.
inline bool
ispfchar(unsigned char c)
{
	return isalnum(c) || c == '.' || c == '_' || c == '-';
}


#endif  // include guard
