// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_CTYPE_ISASCII_H_
#define SHADOW_INCLUDE_LIB_STRING_CTYPE_ISASCII_H_


#include "config.h"

#include <string.h>

#include "string/strcmp/streq.h"


#define CTYPE_CNTRL_C                                                      \
	"\x1F\x1E\x1D\x1C\x1B\x1A\x19\x18\x17\x16\x15\x14\x13\x12\x11\x10" \
	"\x0F\x0E\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01" /*NUL*/

#define CTYPE_LOWER_C   "abcdefghijklmnopqrstuvwxyz"
#define CTYPE_UPPER_C   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CTYPE_DIGIT_C   "0123456789"
#define CTYPE_PUNCT_C   "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define CTYPE_SPACE_C   " \t\n\v\f\r"
#define CTYPE_ALPHA_C   CTYPE_LOWER_C CTYPE_UPPER_C
#define CTYPE_ALNUM_C   CTYPE_ALPHA_C CTYPE_DIGIT_C
#define CTYPE_GRAPH_C   CTYPE_ALNUM_C CTYPE_PUNCT_C
#define CTYPE_PRINT_C   CTYPE_GRAPH_C " "
#define CTYPE_XDIGIT_C  CTYPE_DIGIT_C "abcdefABCDEF"
#define CTYPE_ASCII_C   CTYPE_PRINT_C CTYPE_CNTRL_C /*NUL*/


// isascii_c - is [:ascii:] C-locale
#define isascii_c(c)   (!!strchr(CTYPE_ASCII_C, c))
#define iscntrl_c(c)   (!!strchr(CTYPE_CNTRL_C, c))
#define islower_c(c)   (!streq(strchrnul(CTYPE_LOWER_C, c), ""))
#define isupper_c(c)   (!streq(strchrnul(CTYPE_UPPER_C, c), ""))
#define isdigit_c(c)   (!streq(strchrnul(CTYPE_DIGIT_C, c), ""))
#define ispunct_c(c)   (!streq(strchrnul(CTYPE_PUNCT_C, c), ""))
#define isspace_c(c)   (!streq(strchrnul(CTYPE_SPACE_C, c), ""))
#define isalpha_c(c)   (!streq(strchrnul(CTYPE_ALPHA_C, c), ""))
#define isalnum_c(c)   (!streq(strchrnul(CTYPE_ALNUM_C, c), ""))
#define isgraph_c(c)   (!streq(strchrnul(CTYPE_GRAPH_C, c), ""))
#define isprint_c(c)   (!streq(strchrnul(CTYPE_PRINT_C, c), ""))
#define isxdigit_c(c)  (!streq(strchrnul(CTYPE_XDIGIT_C, c), ""))


#endif  // include guard
