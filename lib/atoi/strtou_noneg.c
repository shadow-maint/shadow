// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/strtou_noneg.h"


extern inline unsigned long strtoul_noneg(const char *s,
    char **restrict endp, int base);
extern inline unsigned long long strtoull_noneg(const char *s,
    char **restrict endp, int base);
