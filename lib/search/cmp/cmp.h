// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_
#define SHADOW_INCLUDE_LIB_SEARCH_CMP_CMP_H_


#include <config.h>


#define CMP(TYPE)                                                     \
(                                                                     \
	_Generic((TYPE) 0,                                            \
		int *:            cmp_int,                            \
		long *:           cmp_long,                           \
		unsigned int *:   cmp_uint,                           \
		unsigned long *:  cmp_ulong                           \
	)                                                             \
)


/* Compatible with bsearch(3), lfind(3), and qsort(3).  */
inline int cmp_int(const void *key, const void *elt);
inline int cmp_long(const void *key, const void *elt);
inline int cmp_uint(const void *key, const void *elt);
inline int cmp_ulong(const void *key, const void *elt);


inline int
cmp_int(const void *key, const void *elt)
{
	const int  *k = key;
	const int  *e = elt;

	if (*k < *e)
		return -1;
	if (*k > *e)
		return +1;
	return 0;
}


inline int
cmp_long(const void *key, const void *elt)
{
	const long  *k = key;
	const long  *e = elt;

	if (*k < *e)
		return -1;
	if (*k > *e)
		return +1;
	return 0;
}


inline int
cmp_uint(const void *key, const void *elt)
{
	const unsigned int  *k = key;
	const unsigned int  *e = elt;

	if (*k < *e)
		return -1;
	if (*k > *e)
		return +1;
	return 0;
}


inline int
cmp_ulong(const void *key, const void *elt)
{
	const unsigned long  *k = key;
	const unsigned long  *e = elt;

	if (*k < *e)
		return -1;
	if (*k > *e)
		return +1;
	return 0;
}


#endif  // include guard
