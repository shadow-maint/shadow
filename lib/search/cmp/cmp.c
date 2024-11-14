// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "search/cmp/cmp.h"


extern inline int cmp_int(const void *key, const void *elt);
extern inline int cmp_long(const void *key, const void *elt);
extern inline int cmp_uint(const void *key, const void *elt);
extern inline int cmp_ulong(const void *key, const void *elt);
