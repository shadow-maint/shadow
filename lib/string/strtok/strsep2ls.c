// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "string/strtok/strsep2ls.h"

#include <stddef.h>
#include <sys/types.h>


extern inline ssize_t strsep2ls(char *s, const char *restrict delim,
    size_t n, char *ls[restrict n]);
