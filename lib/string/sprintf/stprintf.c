// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/sprintf/stprintf.h"

#include <stdarg.h>
#include <sys/types.h>


extern inline int stprintf(ssize_t size;
    char s[restrict size], ssize_t size, const char *restrict fmt, ...);
extern inline int vstprintf(ssize_t size;
    char s[restrict size], ssize_t size, const char *restrict fmt, va_list ap);
