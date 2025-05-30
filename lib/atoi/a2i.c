// SPDX-FileCopyrightText: 2023-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/a2i.h"

#include <sys/types.h>


extern inline int A2I_nc__short(short *restrict n, char *s,
    char **restrict endp, int base, short min, short max);
extern inline int A2I_nc__int(int *restrict n, char *s,
    char **restrict endp, int base, int min, int max);
extern inline int A2I_nc__long(long *restrict n, char *s,
    char **restrict endp, int base, long min, long max);
extern inline int A2I_nc__llong(llong *restrict n, char *s,
    char **restrict endp, int base, llong min, llong max);

extern inline int A2I_nc__u_short(u_short *restrict n, char *s,
    char **restrict endp, int base, u_short min, u_short max);
extern inline int A2I_nc__u_int(u_int *restrict n, char *s,
    char **restrict endp, int base, u_int min, u_int max);
extern inline int A2I_nc__u_long(u_long *restrict n, char *s,
    char **restrict endp, int base, u_long min, u_long max);
extern inline int A2I_nc__u_llong(u_llong *restrict n, char *s,
    char **restrict endp, int base, u_llong min, u_llong max);


extern inline int A2I_c__short(short *restrict n, const char *s,
    const char **restrict endp, int base, short min, short max);
extern inline int A2I_c__int(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max);
extern inline int A2I_c__long(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max);
extern inline int A2I_c__llong(llong *restrict n, const char *s,
    const char **restrict endp, int base, llong min, llong max);

extern inline int A2I_c__u_short(u_short *restrict n, const char *s,
    const char **restrict endp, int base, u_short min, u_short max);
extern inline int A2I_c__u_int(u_int *restrict n, const char *s,
    const char **restrict endp, int base, u_int min, u_int max);
extern inline int A2I_c__u_long(u_long *restrict n, const char *s,
    const char **restrict endp, int base, u_long min, u_long max);
extern inline int A2I_c__u_llong(u_llong *restrict n, const char *s,
    const char **restrict endp, int base, u_llong min, u_llong max);
