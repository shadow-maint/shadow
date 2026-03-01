// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_
#define SHADOW_INCLUDE_LIB_STRING_STRSPN_STPRCSPN_H_


#include "config.h"

#include <string.h>

#include "string/strchr/strnul.h"
#include "string/strspn/strrcspn.h"


// stprcspn - string returns-pointer rear complement span
#define stprcspn(s, reject)                                           \
({                                                                    \
	__auto_type  s_ = (s);                                        \
	                                                              \
	strnul(s_) - strrcspn(s_, reject);                            \
})


#endif  // include guard
