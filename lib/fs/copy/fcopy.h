// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_FS_COPY_FCOPY_H_
#define SHADOW_INCLUDE_LIB_FS_COPY_FCOPY_H_


#include "config.h"

#include <stdio.h>


inline int fcopy(FILE *dst, FILE *src);


// fcopy - FILE copy
inline int
fcopy(FILE *dst, FILE *src)
{
	int  c;

	if (ftello(dst) != 0)
		return -1;
	if (fseek(src, 0, SEEK_SET) == -1)
		return -1;

	while (EOF != (c = fgetc(src))) {
		if (fputc(c, dst) == EOF)
			return -1;
	}
	if (ferror(src))
		return -1;
	if (fflush(dst) == -1)
		return -1;
	return 0;
}


#endif  // include guard
