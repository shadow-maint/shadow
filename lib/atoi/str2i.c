// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/str2i.h"


extern inline int getlong(const char *restrict s, long *restrict n);
extern inline int getulong(const char *restrict s, unsigned long *restrict n);
