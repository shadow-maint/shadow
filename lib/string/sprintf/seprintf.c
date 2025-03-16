// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/sprintf/seprintf.h"

#include <stdarg.h>


#if !defined(HAVE_SEPRINTF)
extern inline char *seprintf(char *dst, const char *end,
    const char *restrict fmt, ...);
extern inline char *vseprintf(char *dst, const char *end,
    const char *restrict fmt, va_list ap);
#endif
