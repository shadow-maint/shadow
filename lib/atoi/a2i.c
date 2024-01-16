// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i.h"


extern inline int a2sh(short *restrict n, const char *s,
    char **restrict endp, int base, short min, short max);
extern inline int a2si(int *restrict n, const char *s,
    char **restrict endp, int base, int min, int max);
extern inline int a2sl(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max);
extern inline int a2sll(long long *restrict n, const char *s,
    char **restrict endp, int base, long long min, long long max);
extern inline int a2uh(unsigned short *restrict n, const char *s,
    char **restrict endp, int base, unsigned short min, unsigned short max);
extern inline int a2ui(unsigned int *restrict n, const char *s,
    char **restrict endp, int base, unsigned int min, unsigned int max);
extern inline int a2ul(unsigned long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long min, unsigned long max);
extern inline int a2ull(unsigned long long *restrict n, const char *s,
    char **restrict endp, int base, unsigned long long min,
    unsigned long long max);
