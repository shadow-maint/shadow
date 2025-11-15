// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_ASTRSEP2LS_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_ASTRSEP2LS_H_


#include "config.h"

#include <stddef.h>

#include "alloc/malloc.h"
#include "attr.h"
#include "exit_if_null.h"
#include "string/strchr/strchrscnt.h"
#include "string/strtok/strsep2ls.h"


// xastrsep2ls - exit-on-error allocate string separate to list-of-strings
#define xastrsep2ls(s, delim, np)  exit_if_null(astrsep2ls(s, delim, np))


ATTR_ACCESS(read_write, 1) ATTR_ACCESS(write_only, 3)
ATTR_STRING(1) ATTR_STRING(2)
inline char **astrsep2ls(char *restrict s, const char *restrict delim,
    size_t *restrict np);


// astrsep2ls - allocate string separate to list-of-strings
inline char **
astrsep2ls(char *s, const char *restrict delim, size_t *restrict np)
{
	char     **ls;
	ssize_t  n;

	n = strchrscnt(s, delim) + 2;

	ls = malloc_T(n, char *);
	if (ls == NULL)
		return NULL;

	n = strsep2ls(s, delim, n, ls);
	if (n == -1) {
		free(ls);
		return NULL;
	}

	if (np != NULL)
		*np = n;

	return ls;
}


#endif  // include guard
