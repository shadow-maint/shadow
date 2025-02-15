// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRSCNT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRSCNT_H_


#include <config.h>

#include <stddef.h>

#include "attr.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"


ATTR_STRING(1)
ATTR_STRING(2)
inline size_t strchrscnt(const char *s, const char *c);


// string characters count
// Similar to strchrcnt(), but search for multiple characters.
inline size_t
strchrscnt(const char *s, const char *c)
{
	size_t  n = 0;

	for (; !streq(c, ""); c++)
		n += strchrcnt(s, *c);

	return n;
}


#endif  // include guard
