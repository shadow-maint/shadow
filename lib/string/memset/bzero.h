// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_MEMSET_BZERO_H_
#define SHADOW_INCLUDE_LIB_STRING_MEMSET_BZERO_H_


#include "config.h"

#include <strings.h>

#include "sizeof.h"


// bzero_a - byte zero array
#define bzero_a(a)  bzero(a, sizeof_a(a))


#endif  // include guard
