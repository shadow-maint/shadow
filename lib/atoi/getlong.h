// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_


#include <config.h>

#include "attr.h"


ATTR_STRING(1)
int getl(const char *numstr, long *result);
ATTR_STRING(1)
int getul(const char *numstr, unsigned long *result);


#endif  // include guard
