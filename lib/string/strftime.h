// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRFTIME_H_
#define SHADOW_INCLUDE_LIB_STRFTIME_H_


#include "config.h"

#include <time.h>

#include "attr.h"
#include "pragma.h"
#include "sizeof.h"


// strftime_a - string format time array
#define strftime_a(dst, fmt, tm)  DEPRECATED(strftime(dst, countof(dst), fmt, tm))


ATTR_DEPRECATED typeof(strftime)  strftime;


#endif  // include guard
