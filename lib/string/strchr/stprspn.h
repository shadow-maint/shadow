// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STPRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STPRSPN_H_


#include <config.h>

#include <string.h>

#include "attr.h"
#include "string/strchr/strnul.h"


// Similar to strspn(3), but start from the end, and return a pointer.
ATTR_STRING(2)
inline char *stprspn(char *restrict s, const char *restrict accept);


inline char *
stprspn(char *restrict s, const char *restrict accept)
{
	char  *p;

	p = strnul(s);
	while (p > s) {
		p--;
		if (strchr(accept, *p) == NULL)
			return p + 1;
	}
	return s;
}


#endif  // include guard
