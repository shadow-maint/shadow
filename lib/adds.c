// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "adds.h"

#include <stddef.h>


extern inline short addsh2(short a, short b);
extern inline long addsl2(long a, long b);

extern inline short addshN(size_t n, short addend[n]);
extern inline long addslN(size_t n, long addend[n]);
