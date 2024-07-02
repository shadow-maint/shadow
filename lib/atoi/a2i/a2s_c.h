// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_C_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2S_C_H_


#include <config.h>

#include <errno.h>
#include <inttypes.h>

#include "atoi/a2i/a2s_nc.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sh_c(short *restrict n, const char *s,
    const char **restrict endp, int base, short min, short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2si_c(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sl_c(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2sll_c(long long *restrict n, const char *s,
    const char **restrict endp, int base, long long min, long long max);


inline int
a2sh_c(short *restrict n, const char *s,
    const char **restrict endp, int base, short min, short max)
{
	return a2sh_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2si_c(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max)
{
	return a2si_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2sl_c(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max)
{
	return a2sl_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2sll_c(long long *restrict n, const char *s,
    const char **restrict endp, int base, long long min, long long max)
{
	return a2sll_nc(n, (char *) s, (char **) endp, base, min, max);
}


#endif  // include guard
