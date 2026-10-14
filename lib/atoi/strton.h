// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTON_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTON_H_


#include "config.h"

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/param.h>

#include "typetraits.h"


#define strtoi_(s, ...)  strton_(intmax_t, QChar_of(s), s, __VA_ARGS__)
#define strtou_(s, ...)  strton_(uintmax_t, QChar_of(s), s, __VA_ARGS__)

#define strton_(T, QChar, ...)                                        \
((static inline T                                                     \
  (const char *s, QChar **endp, int base, T min, T max, int *status)) \
{                                                                     \
	T      n;                                                     \
	int    e, st;                                                 \
	QChar  *end;                                                  \
                                                                      \
	if (endp == NULL)                                             \
		endp = &end;                                          \
	if (status == NULL)                                           \
		status = &st;                                         \
                                                                      \
	if (base != 0 && (base < 2 || base > 36)) {                   \
		*status = EINVAL;                                     \
		return MAX(min, MIN(max, 0));                         \
	}                                                             \
                                                                      \
	e = errno;                                                    \
	errno = 0;                                                    \
                                                                      \
	n = _Generic(n,                                               \
		intmax_t:   strtoimax,                                \
		uintmax_t:  strtoumax                                 \
	)(s, const_cast(char **, endp), base);                        \
                                                                      \
	if (*endp == s)                                               \
		*status = ECANCELED;                                  \
	else if (errno == ERANGE || n < min || n > max)               \
		*status = ERANGE;                                     \
	else if (**endp != '\0')                                      \
		*status = ENOTSUP;                                    \
	else                                                          \
		*status = 0;                                          \
                                                                      \
	errno = e;                                                    \
                                                                      \
	return MAX(min, MIN(max, n));                                 \
}(__VA_ARGS__))


#define strtou_noneg(s, ...)  strtou_noneg_Q(QChar_of(s), s, __VA_ARGS__)

#define strtou_noneg_Q(QChar, ...)                                    \
((static inline uintmax_t                                             \
  (QChar *s, QChar **endp, int base, uintmax_t min, uintmax_t max, int *status))\
{                                                                     \
	int  st;                                                      \
                                                                      \
	if (status == NULL)                                           \
		status = &st;                                         \
	if (strtoi_(s, endp, base, 0, 1, status) == 0 && *status == ERANGE) \
		return min;                                           \
                                                                      \
	return strtou_(s, endp, base, min, max, status);              \
}(__VA_ARGS__))


#endif  // include guard
