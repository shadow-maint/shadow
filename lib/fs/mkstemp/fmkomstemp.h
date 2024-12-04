// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_MKSTEMP_FMKOMSTEMP_H_
#define SHADOW_INCLUDE_LIB_FS_MKSTEMP_FMKOMSTEMP_H_


#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs/mkstemp/mkomstemp.h"


inline FILE *fmkomstemp(char *template, unsigned int flags, mode_t m);


// FILE make with-open(2)-flags with-mode secure temporary
inline FILE *
fmkomstemp(char *template, unsigned int flags, mode_t m)
{
	int   fd;
	FILE  *fp;

	fd = mkomstemp(template, flags, m);
	if (fd == -1)
		return NULL;

	fp = fdopen(fd, "w");
	if (fp == NULL)
		goto fail;

	return fp;
fail:
	close(fd);
	unlink(template);
	return NULL;
}


#endif  // include guard
