// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "memory/memcmp/strneq.h"

#include <stdbool.h>
#include <stddef.h>


extern inline bool strneq(const char *strn, const char *s, size_t size);
