// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_


#include "config.h"

#include <string.h>

#include "string/strspn/strrspn.h"


// stprspn - string offset-pointer rear substring prefix length
// Available in Oracle Solaris as strrspn(3GEN).
// <https://docs.oracle.com/cd/E36784_01/html/E36877/strrspn-3gen.html>
#define stprspn                                                       \
((static inline auto *(auto *s, const char *accept))                  \
{                                                                     \
	return s + strrspn_(s, accept);                               \
})


#endif  // include guard
