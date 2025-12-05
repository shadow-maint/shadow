// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCPY_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCPY_STRTCPY_H_


#include "config.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "attr.h"
#include "sizeof.h"


// strtcpy_a - string truncate copy array
#define strtcpy_a(dst, src)  strtcpy(dst, src, countof(dst))


ATTR_STRING(2)
inline ssize_t strtcpy(char *restrict dst, const char *restrict src,
    size_t dsize);


// strtcpy - string truncate copy
inline ssize_t
strtcpy(char *restrict dst, const char *restrict src, size_t dsize)
{
	bool    trunc;
	size_t  dlen, slen;

	if (dsize == 0)
		abort();

	slen = strnlen(src, dsize);
	trunc = (slen == dsize);
	dlen = slen - trunc;

	stpcpy(mempcpy(dst, src, dlen), "");

	if (trunc) {
		errno = E2BIG;
		return -1;
	}

	return slen;
}


#endif  // include guard
