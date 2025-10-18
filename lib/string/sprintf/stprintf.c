// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/sprintf/stprintf.h"

#include <stdarg.h>


extern inline int stprintf(char *restrict s, int size,
    const char *restrict fmt, ...);
extern inline int vstprintf(char *restrict s, int size,
    const char *restrict fmt, va_list ap);
