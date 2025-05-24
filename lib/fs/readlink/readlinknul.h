// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_READLINK_READLINKNUL_H_
#define SHADOW_INCLUDE_LIB_FS_READLINK_READLINKNUL_H_


#include <config.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "attr.h"
#include "sizeof.h"


#define READLINKNUL(link, buf)  readlinknul(link, buf, countof(buf))


ATTR_STRING(1)
inline ssize_t readlinknul(const char *restrict link, char *restrict buf,
    ssize_t size);


// Similar to readlink(2), but terminate the string.
inline ssize_t
readlinknul(const char *restrict link, char *restrict buf, ssize_t size)
{
	ssize_t  len;

	len = readlink(link, buf, size);
	if (len == -1)
		return -1;

	if (len == size) {
		stpcpy(&buf[size-1], "");
		errno = E2BIG;
		return -1;
	}

	stpcpy(&buf[len], "");

	return len;
}


#endif  // include guard
