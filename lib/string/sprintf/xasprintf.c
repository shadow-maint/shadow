// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/sprintf/xasprintf.h"

#include <stdarg.h>


extern inline int xasprintf(char **restrict s, const char *restrict fmt, ...);
extern inline int xvasprintf(char **restrict s, const char *restrict fmt,
    va_list ap);
