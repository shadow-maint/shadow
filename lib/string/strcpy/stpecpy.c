// SPDX-FileCopyrightText: 2022-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/strcpy/stpecpy.h"


#if !defined(HAVE_STPECPY)
extern inline char *stpecpy(char dst[], char end[];
    char dst[dst ? end - dst : 0], char end[0], const char *restrict src);
#endif
