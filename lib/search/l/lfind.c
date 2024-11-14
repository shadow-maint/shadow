// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "search/l/lfind.h"

#include <stddef.h>


extern inline void *lfind_(const void *k, const void *a, size_t n, size_t ksize,
    typeof(int (const void *k, const void *elt)) *cmp);
