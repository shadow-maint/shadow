// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_STRSEP2ARR_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_STRSEP2ARR_H_


#include <config.h>

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "attr.h"
#include "sizeof.h"


#define STRSEP2ARR(s, delim, a)                                       \
(                                                                     \
	strsep2arr(s, delim, countof(a), a) == countof(a) ? 0 : -1    \
)


ATTR_ACCESS(read_write, 1) ATTR_ACCESS(write_only, 4, 3)
ATTR_STRING(1) ATTR_STRING(2)
inline ssize_t strsep2arr(char *s, const char *restrict delim,
    size_t n, char *a[restrict n]);


// string separate to array-of-strings
// strsep(3) a string into an array of strings.
// Return the number of fields in the string, or -1 on error.
inline ssize_t
strsep2arr(char *s, const char *restrict delim, size_t n, char *a[restrict n])
{
	size_t  i;

	for (i = 0; i < n && s != NULL; i++)
		a[i] = strsep(&s, delim);

	if (s != NULL) {
		errno = E2BIG;
		return -1;
	}

	return i;
}


#endif  // include guard
