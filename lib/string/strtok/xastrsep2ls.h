// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRTOK_XASTRSEP2LS_H_
#define SHADOW_INCLUDE_LIB_STRING_STRTOK_XASTRSEP2LS_H_


#include <config.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attr.h"
#include "shadowlog.h"
#include "string/strtok/astrsep2ls.h"


ATTR_ACCESS(read_write, 1) ATTR_ACCESS(write_only, 3)
ATTR_STRING(1) ATTR_STRING(2)
inline char **xastrsep2ls(char *restrict s, const char *restrict delim,
    size_t *restrict np);


// exit-on-error allocate string separate to list-of-strings
inline char **
xastrsep2ls(char *s, const char *restrict delim, size_t *restrict np)
{
	char     **ls;

	ls = astrsep2ls(s, delim, np);
	if (ls == NULL)
		goto x;

	return ls;
x:
	fprintf(log_get_logfd(), "%s: %s\n",
	        log_get_progname(), strerror(errno));
	exit(13);
}


#endif  // include guard
