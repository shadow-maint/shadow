// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_STPSEP_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_STPSEP_H_


#include <config.h>

#include <string.h>

#include "attr.h"


ATTR_STRING(1) ATTR_STRING(2)
inline char *stpsep(char *s, const char *delim);


// string returns-pointer separate
// Similar to strsep(3),
// but return the next token, and don't update the input pointer.
// Similar to strtok(3),
// but don't store a state, and don't skip empty fields.
inline char *
stpsep(char *s, const char *delim)
{
	strsep(&s, delim);

	return s;
}


#endif  // include guard
