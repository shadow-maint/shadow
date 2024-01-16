// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_H_


#include <config.h>

#include <errno.h>

#include "atoi/strtoi.h"
#include "atoi/strtou_noneg.h"
#include "attr.h"


#define a2i(TYPE, ...)                                                        \
(                                                                             \
	_Generic((TYPE) 0,                                                    \
		short:              a2sh,                                     \
		int:                a2si,                                     \
		long:               a2sl,                                     \
		long long:          a2sll,                                    \
		unsigned short:     a2uh,                                     \
		unsigned int:       a2ui,                                     \
		unsigned long:      a2ul,                                     \
		unsigned long long: a2ull                                     \
	)(__VA_ARGS__)                                                        \
)


ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2sh(const char *s, short *restrict n,
    char **restrict endptr, int base, short min, short max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2si(const char *s, int *restrict n,
    char **restrict endptr, int base, int min, int max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2sl(const char *s, long *restrict n,
    char **restrict endptr, int base, long min, long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2sll(const char *s, long long *restrict n,
    char **restrict endptr, int base, long long min, long long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2uh(const char *s, unsigned short *restrict n,
    char **restrict endptr, int base, unsigned short min, unsigned short max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2ui(const char *s, unsigned int *restrict n,
    char **restrict endptr, int base, unsigned int min, unsigned int max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2ul(const char *s, unsigned long *restrict n,
    char **restrict endptr, int base, unsigned long min, unsigned long max);
ATTR_STRING(1) ATTR_ACCESS(write_only, 2) ATTR_ACCESS(write_only, 3)
inline int a2ull(const char *s, unsigned long long *restrict n,
    char **restrict endptr, int base, unsigned long long min,
    unsigned long long max);


inline int
a2sh(const char *s, short *restrict n, char **restrict endptr,
    int base, short min, short max)
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
a2si(const char *s, int *restrict n, char **restrict endptr,
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
a2sl(const char *s, long *restrict n, char **restrict endptr,
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
a2sll(const char *s, long long *restrict n, char **restrict endptr,
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
a2uh(const char *s, unsigned short *restrict n, char **restrict endptr,
    int base, unsigned short min, unsigned short max)
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
a2ui(const char *s, unsigned int *restrict n, char **restrict endptr,
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
a2ul(const char *s, unsigned long *restrict n, char **restrict endptr,
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
a2ull(const char *s, unsigned long long *restrict n, char **restrict endptr,
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


#endif  // include guard
