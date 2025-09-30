// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "io/fprintf/eprinte.h"

#include <stdarg.h>


extern inline int eprinte(const char *restrict fmt, ...);
extern inline int veprinte(const char *restrict fmt, va_list ap);
