// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_
#define SHADOW_INCLUDE_LIB_ATOI_GETLONG_H_


#include <config.h>

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>

#include "atoi/strtoi.h"
#include "atoi/strtou_noneg.h"
#include "attr.h"


ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getint(const char *s, int *restrict n,
    char **restrict endptr, int base, int min, int max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getlong(const char *s, long *restrict n,
    char **restrict endptr, int base, long min, long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getllong(const char *s, long long *restrict n,
    char **restrict endptr, int base, long long min, long long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getuint(const char *s, unsigned int *restrict n,
    char **restrict endptr, int base, unsigned int min, unsigned int max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getulong(const char *s, unsigned long *restrict n,
    char **restrict endptr, int base, unsigned long min, unsigned long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int getullong(const char *s, unsigned long long *restrict n,
    char **restrict endptr, int base, unsigned long long min,
    unsigned long long max);

ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int geti(const char *restrict s, int *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getl(const char *restrict s, long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getll(const char *restrict s, long long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getui(const char *restrict s, unsigned int *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getul(const char *restrict s, unsigned long *restrict n);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2)
inline int getull(const char *restrict s, unsigned long long *restrict n);


inline int
getint(const char *s, int *restrict n, char **restrict endptr,
    int base, int min, int max)
{
	int  status;

	*n = strtoi_(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
getlong(const char *s, long *restrict n, char **restrict endptr,
    int base, long min, long max)
{
	int  status;

	*n = strtoi_(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
getllong(const char *s, long long *restrict n, char **restrict endptr,
    int base, long long min, long long max)
{
	int  status;

	*n = strtoi_(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
getuint(const char *s, unsigned int *restrict n, char **restrict endptr,
    int base, unsigned int min, unsigned int max)
{
	int  status;

	*n = strtou_noneg(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
getulong(const char *s, unsigned long *restrict n, char **restrict endptr,
    int base, unsigned long min, unsigned long max)
{
	int  status;

	*n = strtou_noneg(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
getullong(const char *s, unsigned long long *restrict n, char **restrict endptr,
    int base, unsigned long long min, unsigned long long max)
{
	int  status;

	*n = strtou_noneg(s, endptr, base, min, max, &status);
	if (status != 0) {
		errno = status;
		return -1;
	}
	return 0;
}


inline int
geti(const char *restrict s, int *restrict n)
{
	return getint(s, n, NULL, 0, INT_MIN, INT_MAX);
}


inline int
getl(const char *restrict s, long *restrict n)
{
	return getlong(s, n, NULL, 0, LONG_MIN, LONG_MAX);
}


inline int
getll(const char *restrict s, long long *restrict n)
{
	return getllong(s, n, NULL, 0, LLONG_MIN, LLONG_MAX);
}


inline int
getui(const char *restrict s, unsigned int *restrict n)
{
	return getuint(s, n, NULL, 0, 0, UINT_MAX);
}


inline int
getul(const char *restrict s, unsigned long *restrict n)
{
	return getulong(s, n, NULL, 0, 0, ULONG_MAX);
}


inline int
getull(const char *restrict s, unsigned long long *restrict n)
{
	return getullong(s, n, NULL, 0, 0, ULLONG_MAX);
}


#endif  // include guard
