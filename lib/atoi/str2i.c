// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/str2i.h"


extern inline int str2sl(long *restrict n, const char *restrict s);
extern inline int str2ul(unsigned long *restrict n, const char *restrict s);
