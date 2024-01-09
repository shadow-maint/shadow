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
inline uintmax_t strtou_noneg(const char *s, char **restrict endp,
    int base, uintmax_t min, uintmax_t max, int *restrict status);

ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline unsigned long strtoul_noneg(const char *s,
    char **restrict endp, int base);


inline uintmax_t
strtou_noneg(const char *s, char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	int  st;

	if (status == NULL)
		status = &st;
	if (strtoi_(s, endp, base, 0, 1, status) == 0 && *status == ERANGE)
		return min;

	return strtou_(s, endp, base, min, max, status);
}


inline unsigned long
strtoul_noneg(const char *s, char **restrict endp, int base)
{
	if (strtol(s, endp, base) < 0) {
		errno = ERANGE;
		return 0;
	}
	return strtoul(s, endp, base);
}


#endif  // include guard
