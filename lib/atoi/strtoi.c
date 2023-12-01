/*
 * SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/param.h>


#define strtoN(str_, endptr_, base_, min_, max_, status_, TYPE)               \
({                                                                            \
	const char  *str = str_;                                              \
	char        **endptr = endptr_;                                       \
	int         base = base_;                                             \
	TYPE        min = min_;                                               \
	TYPE        max = max_;                                               \
	int         *status = status_;                                        \
                                                                              \
	int         errno_saved, s;                                           \
	char        *ep;                                                      \
	TYPE        n;                                                        \
                                                                              \
	errno_saved = errno;                                                  \
                                                                              \
	if (endptr == NULL)                                                   \
		endptr = &ep;                                                 \
	if (status == NULL)                                                   \
		status = &s;                                                  \
                                                                              \
	errno = 0;                                                            \
	n = _Generic((TYPE) 0,                                                \
	             intmax_t:  strtoimax(str, endptr, base),                 \
	             uintmax_t: strtoumax(str, endptr, base));                \
                                                                              \
	if (*endptr == str)                                                   \
		*status = ECANCELED;                                          \
	else if (errno == EINVAL)                                             \
		*status = EINVAL;                                             \
	else if (errno == ERANGE)                                             \
		*status = ERANGE;                                             \
	else if (n < min || n > max)                                          \
		*status = ERANGE;                                             \
	else if (**endptr != '\0')                                            \
		*status = ENOTSUP;                                            \
	else                                                                  \
		*status = 0;                                                  \
                                                                              \
	errno = errno_saved;                                                  \
                                                                              \
	MAX(min, MIN(max, n));                                                \
})


intmax_t
shadow_strtoi(const char *str, char **restrict endptr, int base,
    intmax_t min, intmax_t max, int *restrict status)
{
	return strtoN(str, endptr, base, min, max, status, intmax_t);
}


uintmax_t
shadow_strtou(const char *str, char **restrict endptr, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	return strtoN(str, endptr, base, min, max, status, uintmax_t);
}


#if !defined(HAVE_STRTOI)
extern inline intmax_t strtoi(const char *str, char **end, int base,
    intmax_t min, intmax_t max, int *restrict status);
extern inline uintmax_t strtou(const char *str, char **end, int base,
    uintmax_t min, uintmax_t max, int *restrict status);
#endif
