// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMCMP_MEMEQ_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMCMP_MEMEQ_H_


#include "config.h"

#include <memory.h>


// memeq - memory equal
#define memeq(a, b, n)  (!memcmp(a, b, n))


#endif  // include guard
