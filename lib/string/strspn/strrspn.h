// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STRRSPN_H_


#include <config.h>

#include <string.h>

#include "attr.h"
#include "string/strchr/strnul.h"


ATTR_STRING(2)
inline char *strrspn(char *restrict s, const char *restrict accept);


// Available in Oracle Solaris: strrspn(3GEN).
// <https://docs.oracle.com/cd/E36784_01/html/E36877/strrspn-3gen.html>
inline char *
strrspn(char *restrict s, const char *restrict accept)
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
