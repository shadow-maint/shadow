// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_NC_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_NC_H_


#include <config.h>

#include <errno.h>

#include "atoi/strtoi/strtoi.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sh_nc(short *restrict n, char *s,
    char **restrict endp, int base, short min, short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2si_nc(int *restrict n, char *s,
    char **restrict endp, int base, int min, int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sl_nc(long *restrict n, char *s,
    char **restrict endp, int base, long min, long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sll_nc(long long *restrict n, char *s,
    char **restrict endp, int base, long long min, long long max);


inline int
a2sh_nc(short *restrict n, char *s,
    char **restrict endp, int base, short min, short max)
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
a2si_nc(int *restrict n, char *s,
    char **restrict endp, int base, int min, int max)
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
a2sl_nc(long *restrict n, char *s,
    char **restrict endp, int base, long min, long max)
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
a2sll_nc(long long *restrict n, char *s,
    char **restrict endp, int base, long long min, long long max)
{
	int  status;

	*n = strtoi_(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


#endif  // include guard
