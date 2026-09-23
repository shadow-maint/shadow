// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMCMP_MEMEQ_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMCMP_MEMEQ_H_


#include "config.h"

#include <memory.h>

#include "attr.h"
#include "pragma.h"


// memeq - memory equal
#define memeq(a, b, n)  (DEPRECATED(memcmp(a, b, n)) == 0)


ATTR_DEPRECATED typeof(memcmp)  memcmp;


#endif  // include guard
