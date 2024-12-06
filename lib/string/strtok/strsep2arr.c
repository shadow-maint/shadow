// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/strtok/strsep2arr.h"

#include <stddef.h>
#include <sys/types.h>


extern inline ssize_t strsep2arr(char *s, const char *restrict delim,
    size_t n, char *a[restrict n]);
