// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_NC_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_NC_H_


#include <config.h>

#include <errno.h>

#include "atoi/strtoi/strtou_noneg.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2uh_nc(unsigned short *restrict n, char *s,
    char **restrict endp, int base, unsigned short min, unsigned short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ui_nc(unsigned int *restrict n, char *s,
    char **restrict endp, int base, unsigned int min, unsigned int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ul_nc(unsigned long *restrict n, char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ull_nc(unsigned long long *restrict n, char *s,
    char **restrict endp, int base, unsigned long long min,
    unsigned long long max);


inline int
a2uh_nc(unsigned short *restrict n, char *s,
    char **restrict endp, int base, unsigned short min,
    unsigned short max)
{
	int  status;

	*n = strtou_noneg(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
a2ui_nc(unsigned int *restrict n, char *s,
    char **restrict endp, int base, unsigned int min, unsigned int max)
{
	int  status;

	*n = strtou_noneg(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
a2ul_nc(unsigned long *restrict n, char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max)
{
	int  status;

	*n = strtou_noneg(s, endp, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
a2ull_nc(unsigned long long *restrict n, char *s,
    char **restrict endp, int base, unsigned long long min,
    unsigned long long max)
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
