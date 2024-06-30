// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i/a2s_c.h"


extern inline int a2sh_c(short *restrict n, const char *s,
    const char **restrict endp, int base, short min, short max);
extern inline int a2si_c(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max);
extern inline int a2sl_c(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max);
extern inline int a2sll_c(long long *restrict n, const char *s,
    const char **restrict endp, int base, long long min, long long max);
