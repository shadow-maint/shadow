// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i/a2u_c.h"


extern inline int a2uh_c(unsigned short *restrict n, const char *s,
    const char **restrict endp, int base, unsigned short min,
    unsigned short max);
extern inline int a2ui_c(unsigned int *restrict n, const char *s,
    const char **restrict endp, int base, unsigned int min, unsigned int max);
extern inline int a2ul_c(unsigned long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long min, unsigned long max);
extern inline int a2ull_c(unsigned long long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long long min,
    unsigned long long max);
