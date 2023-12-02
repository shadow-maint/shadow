/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTOI_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTOI_H_


#include <config.h>

#include <inttypes.h>
#include <stdint.h>

#include "defines.h"
#include "types.h"


#define strton(str, end, base, min, max, status, TYPE)                        \
(                                                                             \
	__builtin_choose_expr(is_signed(TYPE),                                \
	                      strtoi(str, end, base, min, max, status),       \
	                      strtou(str, end, base, min, max, status))       \
)


ATTR_STRING(1)
intmax_t shadow_strtoi(const char *str, char **restrict end, int base,
    intmax_t min, intmax_t max, int *restrict status);
ATTR_STRING(1)
uintmax_t shadow_strtou(const char *str, char **restrict end, int base,
    uintmax_t min, uintmax_t max, int *restrict status);


#if !defined(HAVE_STRTOI)
ATTR_STRING(1)
inline intmax_t strtoi(const char *str, char **restrict end, int base,
    intmax_t min, intmax_t max, int *restrict status);
ATTR_STRING(1)
inline uintmax_t strtou(const char *str, char **restrict end, int base,
    uintmax_t min, uintmax_t max, int *restrict status);


inline intmax_t
strtoi(const char *str, char **restrict end, int base,
    intmax_t min, intmax_t max, int *restrict status)
{
	return shadow_strtoi(str, end, base, min, max, status);
}


inline uintmax_t
strtou(const char *str, char **restrict end, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	return shadow_strtou(str, end, base, min, max, status);
}
#endif


#endif  // include guard
