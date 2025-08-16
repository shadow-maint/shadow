// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "search/l/lsearch.h"

#include <stddef.h>
#include <sys/types.h>


extern inline void LSEARCH__int(size_t *n;
    const int *k, int a[*n], size_t *n);
extern inline void LSEARCH__long(size_t *n;
    const long *k, long a[*n], size_t *n);
extern inline void LSEARCH__u_int(size_t *n;
    const u_int *k, u_int a[*n], size_t *n);
extern inline void LSEARCH__u_long(size_t *n;
    const u_long *k, u_long a[*n], size_t *n);
