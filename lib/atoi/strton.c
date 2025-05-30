// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/strton.h"

#include <stdint.h>


extern inline intmax_t STRTON__intmax_t(const char *s,
    char **restrict endp, int base,
    intmax_t min, intmax_t max, int *restrict status);
extern inline uintmax_t STRTON__uintmax_t(const char *s,
    char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status);

extern inline uintmax_t strtou_noneg(const char *s,
    char **restrict endp, int base,
    uintmax_t min, uintmax_t max, int *restrict status);
