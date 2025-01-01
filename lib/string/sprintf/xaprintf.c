// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/sprintf/xaprintf.h"

#include <stdarg.h>


extern inline char *xaprintf(const char *restrict fmt, ...);
extern inline char *xvaprintf(const char *restrict fmt, va_list ap);
