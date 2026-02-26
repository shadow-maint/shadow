// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STREQ_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STREQ_H_


#include "config.h"

#include <string.h>


// streq - strings equal
#define streq(s1, s2)  (!strcmp(s1, s2))


#endif  // include guard
