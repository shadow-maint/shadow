// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/strtoi/strtou.h"

#include <stdint.h>


extern inline uintmax_t strtou_(const char *s, char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status);
