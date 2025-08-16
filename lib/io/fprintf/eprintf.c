// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "io/fprintf/eprintf.h"

#include <stdarg.h>


extern inline int eprintf(const char *restrict fmt, ...);
extern inline int veprintf(const char *restrict fmt, va_list ap);
