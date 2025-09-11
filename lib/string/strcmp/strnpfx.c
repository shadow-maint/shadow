// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/strcmp/strnpfx.h"

#include <stdbool.h>
#include <stddef.h>


extern inline bool strnpfx(const char *strn, const char *prefix, size_t size);
