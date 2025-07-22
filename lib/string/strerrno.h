// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRERRNO_H_
#define SHADOW_INCLUDE_LIB_STRING_STRERRNO_H_


#include "config.h"

#include <errno.h>
#include <string.h>


// strerrno - string errno
#define strerrno()  ((const char *){strerror(errno)})


#endif  // include guard
