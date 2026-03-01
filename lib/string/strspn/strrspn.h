// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRSPN_H_


#include "config.h"

#include <stddef.h>
#include <string.h>

#include "attr.h"
#include "string/strchr/strnul.h"


ATTR_STRING(1)
ATTR_STRING(2)
inline size_t strrspn_(const char *s, const char *accept);


// strrspn_ - string rear span
inline size_t
strrspn_(const char *s, const char *accept)
{
	size_t      spn;
	const char  *p;

	p = strnul(s);
	for (spn = 0; p > s; spn++) {
		p--;
		if (strchr(accept, *p) == NULL)
			return spn;
	}
	return spn;
}


#endif  // include guard
