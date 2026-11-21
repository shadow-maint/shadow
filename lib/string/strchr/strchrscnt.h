// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRSCNT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRSCNT_H_


#include "config.h"

#include <stddef.h>
#include <string.h>

#include "attr.h"


ATTR_STRING(1)
ATTR_STRING(2)
inline size_t strchrscnt(const char *s, const char *c);


// strchrscnt - string characters count
inline size_t
strchrscnt(const char *s, const char *c)
{
	size_t  n = 0;

	while (NULL != (s = strpbrk(s, c))) {
		n++;
		s++;
	}

	return n;
}


#endif  // include guard
