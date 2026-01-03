// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_STRISASCII_H_


#include "config.h"

#include "string/ctype/isascii.h"
#include "string/strcmp/streq.h"
#include "string/strspn/stpspn.h"


// strisascii_c - string is [:ascii:] C-locale
#define strisdigit_c(s)   streq(stpspn(s, CTYPE_DIGIT_C), "")
#define strisprint_c(s)   streq(stpspn(s, CTYPE_PRINT_C), "")


#endif  // include guard
