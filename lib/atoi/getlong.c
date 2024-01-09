// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/getlong.h"


extern inline int getshort(const char *s, short *restrict n,
    char **restrict endptr, int base, short min, short max);
extern inline int getint(const char *s, int *restrict n,
    char **restrict endptr, int base, int min, int max);
extern inline int getlong(const char *s, long *restrict n,
    char **restrict endptr, int base, long min, long max);
extern inline int getllong(const char *s, long long *restrict n,
    char **restrict endptr, int base, long long min, long long max);
extern inline int getushort(const char *s, unsigned short *restrict n,
    char **restrict endptr, int base, unsigned short min, unsigned short max);
extern inline int getuint(const char *s, unsigned int *restrict n,
    char **restrict endptr, int base, unsigned int min, unsigned int max);
extern inline int getulong(const char *s, unsigned long *restrict n,
    char **restrict endptr, int base, unsigned long min, unsigned long max);
extern inline int getullong(const char *s, unsigned long long *restrict n,
    char **restrict endptr, int base, unsigned long long min,
    unsigned long long max);

extern inline int geth(const char *restrict s, short *restrict n);
extern inline int geti(const char *restrict s, int *restrict n);
extern inline int getl(const char *restrict s, long *restrict n);
extern inline int getll(const char *restrict s, long long *restrict n);
extern inline int getuh(const char *restrict s, unsigned short *restrict n);
extern inline int getui(const char *restrict s, unsigned int *restrict n);
extern inline int getul(const char *restrict s, unsigned long *restrict n);
extern inline int getull(const char *restrict s, unsigned long long *restrict n);
