// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/str2i.h"


extern inline int str2sh(const char *restrict s, short *restrict n);
extern inline int str2si(const char *restrict s, int *restrict n);
extern inline int str2sl(const char *restrict s, long *restrict n);
extern inline int str2sll(const char *restrict s, long long *restrict n);
extern inline int str2uh(const char *restrict s, unsigned short *restrict n);
extern inline int str2ui(const char *restrict s, unsigned int *restrict n);
extern inline int str2ul(const char *restrict s, unsigned long *restrict n);
extern inline int str2ull(const char *restrict s, unsigned long long *restrict n);
