// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "string/strcpy/strtcat.h"

#include <stddef.h>


extern inline ssize_t strtcat(char *restrict dst, const char *restrict src,
    size_t dsize);
