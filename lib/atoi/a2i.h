// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_H_


#include <config.h>

#include <errno.h>

#include "atoi/strtoi.h"
#include "atoi/strtou_noneg.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sl(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ul(unsigned long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);


inline int
a2sl(long *restrict n, const char *s, char **restrict endp,
    int base, long min, long max)
{
	int  status;

	*n = strtoi_(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
a2ul(unsigned long *restrict n, const char *s, char **restrict endp,
    int base, unsigned long min, unsigned long max)
{
	int  status;

	*n = strtou_noneg(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


#endif  // include guard
