// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i.h"


extern inline int a2sl(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max);
extern inline int a2ul(unsigned long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);
