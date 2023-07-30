/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIBMISC_SIZEOF_H_
#define SHADOW_INCLUDE_LIBMISC_SIZEOF_H_


#include <config.h>

#include <limits.h>


#define WIDTHOF(x)   (sizeof(x) * CHAR_BIT)
#define NITEMS(arr)  (sizeof((arr)) / sizeof((arr)[0]))
#define STRLEN(s)    (NITEMS(s) - 1)


#endif  // include guard
