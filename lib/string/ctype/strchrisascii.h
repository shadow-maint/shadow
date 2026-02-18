// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRCHRISASCII_H_


#include "config.h"

#include <string.h>

#include "string/ctype/isascii.h"


// strchriscntrl_c - string character is [:cntrl:] C-locale
#define strchriscntrl_c(s)  (!!strpbrk(s, CTYPE_CNTRL_C))


#endif  // include guard
