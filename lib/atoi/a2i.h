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


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sh(short *restrict n, const char *s,
    char **restrict endp, int base, short min, short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2si(int *restrict n, const char *s,
    char **restrict endp, int base, int min, int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sl(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sll(long long *restrict n, const char *s,
    char **restrict endp, int base, long long min, long long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2uh(unsigned short *restrict n, const char *s,
    char **restrict endp, int base, unsigned short min, unsigned short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ui(unsigned int *restrict n, const char *s,
    char **restrict endp, int base, unsigned int min, unsigned int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ul(unsigned long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ull(unsigned long long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long long min,
    unsigned long long max);


inline int
a2sh(short *restrict n, const char *s, char **restrict endp,
    int base, short min, short max)
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
a2si(int *restrict n, const char *s, char **restrict endp,
    int base, int min, int max)
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
a2sll(long long *restrict n, const char *s, char **restrict endp,
    int base, long long min, long long max)
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
a2uh(unsigned short *restrict n, const char *s, char **restrict endp,
    int base, unsigned short min, unsigned short max)
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
a2ui(unsigned int *restrict n, const char *s, char **restrict endp,
    int base, unsigned int min, unsigned int max)
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


inline int
a2ull(unsigned long long *restrict n, const char *s, char **restrict endp,
    int base, unsigned long long min, unsigned long long max)
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
