// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_


#include <config.h>

#include <errno.h>
#include <stdlib.h>

#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long strtoul_noneg(const char *nptr,
    char **restrict endptr, int base);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long long strtoull_noneg(const char *nptr,
    char **restrict endptr, int base);


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
