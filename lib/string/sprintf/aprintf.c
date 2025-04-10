// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/sprintf/aprintf.h"

#include <stdarg.h>


extern inline char *aprintf(const char *restrict fmt, ...);
extern inline char *vaprintf(const char *restrict fmt, va_list ap);
