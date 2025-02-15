// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRSPN_H_


#include <config.h>

#include <string.h>

#include "string/strspn/strrspn.h"


// string returns-pointer rear substring prefix length
// Available in Oracle Solaris as strrspn(3GEN).
// <https://docs.oracle.com/cd/E36784_01/html/E36877/strrspn-3gen.html>
#define stprspn(s, accept)                                                    \
({                                                                            \
	__auto_type  s_ = (s);                                                \
                                                                              \
	s_ + strrspn_(s_, accept);                                            \
})


#endif  // include guard
