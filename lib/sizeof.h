// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIBMISC_SIZEOF_H_
#define SHADOW_INCLUDE_LIBMISC_SIZEOF_H_


#include <config.h>

#include <limits.h>
#if __has_include(<stdcountof.h>)
# include <stdcountof.h>
#endif
#include <sys/types.h>


#define ssizeof(x)           ((ssize_t) sizeof(x))
#define memberof(T, member)  ((T){}.member)
#define WIDTHOF(x)           (sizeof(x) * CHAR_BIT)

#if !defined(countof)
# define countof(a)          (sizeof(a) / sizeof((a)[0]))
#endif

#define SIZEOF_ARRAY(a)      (countof(a) * sizeof((a)[0]))
#define STRLEN(s)            (countof("" s "") - 1)


#endif  // include guard
