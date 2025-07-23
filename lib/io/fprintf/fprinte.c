// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "io/fprintf/fprinte.h"

#include <stdarg.h>
#include <stdio.h>


extern inline int fprinte(FILE *restrict stream, const char *restrict fmt,
    ...);
extern inline int vfprinte(FILE *restrict stream, const char *restrict fmt,
    va_list ap);
