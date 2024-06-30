// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i/a2s_nc.h"


extern inline int a2sh_nc(short *restrict n, char *s,
    char **restrict endp, int base, short min, short max);
extern inline int a2si_nc(int *restrict n, char *s,
    char **restrict endp, int base, int min, int max);
extern inline int a2sl_nc(long *restrict n, char *s,
    char **restrict endp, int base, long min, long max);
extern inline int a2sll_nc(long long *restrict n, char *s,
    char **restrict endp, int base, long long min, long long max);
