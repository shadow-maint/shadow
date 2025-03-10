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

#include "attr.h"


#define strton(T, ...)  (STRTON(T)(__VA_ARGS__))
#define STRTON(T)                                                     \
(                                                                     \
	_Generic((T) 0,                                               \
		intmax_t:   STRTON__ ## intmax_t,                     \
		uintmax_t:  STRTON__ ## uintmax_t                     \
	)                                                             \
)


ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 6)
inline uintmax_t strtou_noneg(const char *s, char **restrict endp,
    int base, uintmax_t min, uintmax_t max, int *restrict status);


#define template_STRTON(T)                                            \
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 6)  \
inline T                                                              \
STRTON__ ## T(const char *s, char **restrict endp, int base,          \
    T min, T max, int *restrict status)                               \
{                                                                     \
	T     n;                                                      \
	int   e, st;                                                  \
	char  *end;                                                   \
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
	)(s, endp, base);                                             \
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
}
template_STRTON(intmax_t);
template_STRTON(uintmax_t);
#undef template_STRTON


inline uintmax_t
strtou_noneg(const char *s, char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	int  st;

	if (status == NULL)
		status = &st;
	if (strton(intmax_t, s, endp, base, 0, 1, status) == 0 && *status == ERANGE)
		return min;

	return strton(uintmax_t, s, endp, base, min, max, status);
}


#endif  // include guard
