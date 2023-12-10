// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTOU_NONEG_H_


#include <config.h>

#include <errno.h>
#include <stdlib.h>

#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long strtoul_noneg(const char *s,
    char **restrict endp, int base);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long long strtoull_noneg(const char *s,
    char **restrict endp, int base);


inline unsigned long
strtoul_noneg(const char *s, char **restrict endp, int base)
{
	if (strtol(s, endp, base) < 0) {
		errno = ERANGE;
		return 0;
	}
	return strtoul(s, endp, base);
}


inline unsigned long long
strtoull_noneg(const char *s, char **restrict endp, int base)
{
	if (strtol(s, endp, base) < 0) {
		errno = ERANGE;
		return 0;
	}
	return strtoull(s, endp, base);
}


#endif  // include guard
