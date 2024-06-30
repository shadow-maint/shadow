// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i/a2u_nc.h"


extern inline int a2uh_nc(unsigned short *restrict n, char *s,
    char **restrict endp, int base, unsigned short min, unsigned short max);
extern inline int a2ui_nc(unsigned int *restrict n, char *s,
    char **restrict endp, int base, unsigned int min, unsigned int max);
extern inline int a2ul_nc(unsigned long *restrict n, char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);
extern inline int a2ull_nc(unsigned long long *restrict n, char *s,
    char **restrict endp, int base, unsigned long long min,
    unsigned long long max);
