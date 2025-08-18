// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STPNPFX_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STPNPFX_H_


#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "attr.h"
#include "sizeof.h"


#define STRNPFX(strn, pfx)  strnpfx(strn, pfx, countof(strn))


ATTR_STRING(2)
inline bool strnpfx(const char *strn, const char *pfx, size_t size);


// nonstring prefix
inline bool
strnpfx(const char *strn, const char *pfx, size_t size)
{
	if (strlen(pfx) > size)
		return false;

	return strncmp(strn, pfx, strlen(pfx)) == 0;
}


#endif  // include guard
