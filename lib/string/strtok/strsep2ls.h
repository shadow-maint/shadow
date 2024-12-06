// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_STRSEP2LS_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_STRSEP2LS_H_


#include <config.h>

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include "attr.h"
#include "sizeof.h"
#include "string/strtok/strsep2arr.h"


#define STRSEP2LS(s, delim, ls)  strsep2ls(s, delim, countof(ls), ls)


ATTR_ACCESS(read_write, 1) ATTR_ACCESS(write_only, 4, 3)
ATTR_STRING(1) ATTR_STRING(2)
inline ssize_t strsep2ls(char *s, const char *restrict delim,
    size_t n, char *ls[restrict n]);


// string separate to list-of-strings
// Like strsep2arr(), but add a NULL terminator.
inline ssize_t
strsep2ls(char *s, const char *restrict delim, size_t n, char *ls[restrict n])
{
	size_t  i;

	i = strsep2arr(s, delim, n, ls);
	if (i >= n) {
		errno = E2BIG;
		return -1;
	}

	ls[i] = NULL;

	return i;
}


#endif  // include guard
