// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i.h"


extern inline int a2sh(const char *s, short *restrict n,
    char **restrict endptr, int base, short min, short max);
extern inline int a2si(const char *s, int *restrict n,
    char **restrict endptr, int base, int min, int max);
extern inline int a2sl(const char *s, long *restrict n,
    char **restrict endptr, int base, long min, long max);
extern inline int a2sll(const char *s, long long *restrict n,
    char **restrict endptr, int base, long long min, long long max);
extern inline int a2uh(const char *s, unsigned short *restrict n,
    char **restrict endptr, int base, unsigned short min, unsigned short max);
extern inline int a2ui(const char *s, unsigned int *restrict n,
    char **restrict endptr, int base, unsigned int min, unsigned int max);
extern inline int a2ul(const char *s, unsigned long *restrict n,
    char **restrict endptr, int base, unsigned long min, unsigned long max);
extern inline int a2ull(const char *s, unsigned long long *restrict n,
    char **restrict endptr, int base, unsigned long long min,
    unsigned long long max);
