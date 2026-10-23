// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRCNT_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRCHRCNT_H_


#include "config.h"

#include <stddef.h>
#include <string.h>

#include "attr.h"


ATTR_STRING(1)
inline size_t strchrcnt(const char *s, char c);


// strchrcnt - string character count
inline size_t
strchrcnt(const char *s, char c)
{
	size_t  n = 0;

	while (NULL != (s = strchr(s, c))) {
		n++;
		s++;
	}

	return n;
}


#endif  // include guard
