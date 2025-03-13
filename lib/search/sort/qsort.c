// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "search/sort/qsort.h"

#include <stddef.h>
#include <sys/types.h>


extern inline void QSORT__int(size_t n;
    int a[n], size_t n);
extern inline void QSORT__long(size_t n;
    long a[n], size_t n);
extern inline void QSORT__u_int(size_t n;
    u_int a[n], size_t n);
extern inline void QSORT__u_long(size_t n;
    u_long a[n], size_t n);
