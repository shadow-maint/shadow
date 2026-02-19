// SPDX-FileCopyrightText: 2024-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCHR_STRNUL_H_


#include "config.h"

#include <string.h>


// strnul - string null-byte
#define strnul(s)  strchr(s, '\0')


#endif  // include guard
