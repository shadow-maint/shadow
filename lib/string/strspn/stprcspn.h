// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_


#include <config.h>

#include <string.h>

#include "string/strspn/strrcspn.h"


// string returns-pointer rear complement substring prefix length
#define stprcspn(s, reject)                                                   \
({                                                                            \
	__auto_type  s_ = (s);                                                \
                                                                              \
	s_ + strrcspn(s_, reject);                                            \
})


#endif  // include guard
