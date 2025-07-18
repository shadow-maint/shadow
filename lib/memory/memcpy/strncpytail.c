// SPDX-FileCopyrightText: 2025-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "memory/memcpy/strncpytail.h"

#include <stddef.h>


extern inline char *strncpytail(char *restrict dst, const char *restrict src,
    size_t dsize);
