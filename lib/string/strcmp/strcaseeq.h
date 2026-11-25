// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEEQ_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STRCASEEQ_H_


#include "config.h"

#include <strings.h>

#include "attr.h"
#include "pragma.h"


// strcaseeq - strings case-insensitive equal
#define strcaseeq(s1, s2)  (DEPRECATED(strcasecmp(s1, s2)) == 0)


ATTR_DEPRECATED typeof(strcasecmp)  strcasecmp;


#endif  // include guard
