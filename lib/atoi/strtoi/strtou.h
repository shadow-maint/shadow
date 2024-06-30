// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_STRTOI_STRTOU_H_
#define SHADOW_INCLUDE_LIB_ATOI_STRTOI_STRTOU_H_


#include <config.h>

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/param.h>

#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 6)
inline uintmax_t strtou_(const char *s, char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status);


inline uintmax_t
strtou_(const char *s, char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status)
{
	int        e, st;
	char       *end;
	uintmax_t  n;

	if (endp == NULL)
		endp = &end;
	if (status == NULL)
		status = &st;

	if (base != 0 && (base < 2 || base > 36)) {
		*status = EINVAL;
		return MAX(min, 0);
	}

	e = errno;
	errno = 0;

	n = strtoumax(s, endp, base);

	if (*endp == s)
		*status = ECANCELED;
	else if (errno == ERANGE || n < min || n > max)
		*status = ERANGE;
	else if (**endp != '\0')
		*status = ENOTSUP;
	else
		*status = 0;

	errno = e;

	return MAX(min, MIN(max, n));
}


#endif  // include guard
