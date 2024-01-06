// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/getlong.h"


extern inline int getlong(const char *s, long *restrict n,
    char **restrict endptr, int base, long min, long max);
extern inline int getulong(const char *s, unsigned long *restrict n,
    char **restrict endptr, int base, unsigned long min, unsigned long max);

extern inline int getl(const char *restrict s, long *restrict n);
extern inline int getul(const char *restrict s, unsigned long *restrict n);
