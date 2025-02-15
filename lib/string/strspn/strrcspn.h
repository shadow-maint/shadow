// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRCSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRCSPN_H_


#include <config.h>

#include <stddef.h>
#include <string.h>

#include "attr.h"
#include "string/strchr/strnul.h"


ATTR_STRING(1)
ATTR_STRING(2)
inline size_t strrcspn(const char *s, const char *reject);


// string rear complement substring prefix length
inline size_t
strrcspn(const char *s, const char *reject)
{
	char  *p;

	p = strnul(s);
	while (p > s) {
		p--;
		if (strchr(reject, *p) != NULL)
			return p + 1 - s;
	}
	return 0;
}


#endif  // include guard
