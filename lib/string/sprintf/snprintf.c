// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/sprintf/snprintf.h"

#include <stdarg.h>
#include <stddef.h>


extern inline int snprintf_(size_t size;
    char s[restrict size], size_t size, const char *restrict fmt, ...);
extern inline int vsnprintf_(size_t size;
    char s[restrict size], size_t size, const char *restrict fmt, va_list ap);
