// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_C_H_
#define SHADOW_INCLUDE_LIB_ATOI_A2I_A2U_C_H_


#include <config.h>

#include "atoi/a2i/a2u_nc.h"
#include "attr.h"


ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2uh_c(unsigned short *restrict n, const char *s,
    const char **restrict endp, int base, unsigned short min,
    unsigned short max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ui_c(unsigned int *restrict n, const char *s,
    const char **restrict endp, int base, unsigned int min, unsigned int max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ul_c(unsigned long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long min, unsigned long max);
ATTR_STRING(2) ATTR_ACCESS(write_only, 1) ATTR_ACCESS(write_only, 3)
inline int a2ull_c(unsigned long long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long long min,
    unsigned long long max);


inline int
a2uh_c(unsigned short *restrict n, const char *s,
    const char **restrict endp, int base, unsigned short min,
    unsigned short max)
{
	return a2uh_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2ui_c(unsigned int *restrict n, const char *s,
    const char **restrict endp, int base, unsigned int min, unsigned int max)
{
	return a2ui_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2ul_c(unsigned long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long min, unsigned long max)
{
	return a2ul_nc(n, (char *) s, (char **) endp, base, min, max);
}


inline int
a2ull_c(unsigned long long *restrict n, const char *s,
    const char **restrict endp, int base, unsigned long long min,
    unsigned long long max)
{
	return a2ull_nc(n, (char *) s, (char **) endp, base, min, max);
}


#endif  // include guard
