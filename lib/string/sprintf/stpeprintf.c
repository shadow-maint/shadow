// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/sprintf/stpeprintf.h"

#include <stdarg.h>


#if !defined(HAVE_STPEPRINTF)
extern inline char *stpeprintf(char dst[], char end[];
    char dst[dst ? end - dst : 0], char end[0], const char *restrict fmt, ...);
extern inline char *vstpeprintf(char dst[], char end[];
    char dst[dst ? end - dst : 0], char end[0], const char *restrict fmt, va_list ap);
#endif
