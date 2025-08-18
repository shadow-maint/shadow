// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/strcmp/strneq.h"

#include <stdbool.h>
#include <stddef.h>


extern inline bool strneq(const char *strn, const char *s, size_t size);
