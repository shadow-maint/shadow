// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "io/fprintf.h"

#include <stdarg.h>
#include <stdio.h>


extern inline int fprintec(FILE *restrict stream, int e,
    const char *restrict fmt, ...);
extern inline int vfprintec(FILE *restrict stream, int e,
    const char *restrict fmt, va_list ap);
