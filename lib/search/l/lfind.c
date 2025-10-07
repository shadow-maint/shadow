// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "search/l/lfind.h"

#include <stddef.h>
#include <sys/types.h>


extern inline const int *LFIND__int(size_t n;
    const int *k, const int a[n], size_t n);
extern inline const long *LFIND__long(size_t n;
    const long *k, const long a[n], size_t n);
extern inline const u_int *LFIND__u_int(size_t n;
    const u_int *k, const u_int a[n], size_t n);
extern inline const u_long *LFIND__u_long(size_t n;
    const u_long *k, const u_long a[n], size_t n);
