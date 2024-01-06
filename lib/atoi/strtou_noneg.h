// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_


#include <config.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "atoi/strtoi.h"
#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 6)
inline uintmax_t strtou_noneg(const char *nptr, char **restrict endptr,
    int base, uintmax_t min, uintmax_t max, int *restrict status);

ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long strtoul_noneg(const char *nptr,
    char **restrict endptr, int base);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long long strtoull_noneg(const char *nptr,
    char **restrict endptr, int base);


inline uintmax_t
strtou_noneg(const char *nptr, char **restrict endptr, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	int  st;

	if (status == NULL)
		status = &st;
	if (strtoi_(nptr, endptr, base, 0, 1, status) == 0 && *status == ERANGE)
		return min;

	return strtou_(nptr, endptr, base, min, max, status);
}


inline unsigned long
strtoul_noneg(const char *nptr, char **restrict endptr, int base)
{
	if (strtol(nptr, endptr, base) < 0) {
		errno = ERANGE;
		return 0;
	}
	return strtoul(nptr, endptr, base);
}


inline unsigned long long
strtoull_noneg(const char *nptr, char **restrict endptr, int base)
{
	if (strtol(nptr, endptr, base) < 0) {
		errno = ERANGE;
		return 0;
	}
	return strtoull(nptr, endptr, base);
}


#endif  // include guard
