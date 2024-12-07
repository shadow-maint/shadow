// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/strtok/xastrsep2ls.h"

#include <stddef.h>


extern inline char **xastrsep2ls(char *restrict s, const char *restrict delim,
    size_t *restrict np);
