// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_MKSTEMP_MKOMSTEMP_H_
#define SHADOW_INCLUDE_LIB_FS_MKSTEMP_MKOMSTEMP_H_


#include <config.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


inline int mkomstemp(char *template, unsigned int flags, mode_t m);


// make with-open(2)-like-flags with-mode secure temporary
inline int
mkomstemp(char *template, unsigned int flags, mode_t m)
{
	int  fd;

	fd = mkostemp(template, flags);
	if (fd == -1)
		return -1;

	if (fchmod(fd, m) == -1)
		goto fail;

	return fd;
fail:
	close(fd);
	unlink(template);
	return -1;
}


#endif  // include guard
