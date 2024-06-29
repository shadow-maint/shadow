// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_ALLOC_REALLOCF_H_
#define SHADOW_INCLUDE_LIB_ALLOC_REALLOCF_H_


#include <config.h>

#include <stddef.h>
#include <stdlib.h>

#include "attr.h"


#define REALLOCF(ptr, n, type)                                                \
(                                                                             \
	_Generic(ptr, type *:  (type *) reallocarrayf(ptr, n, sizeof(type)))  \
)


ATTR_ALLOC_SIZE(2, 3)
ATTR_MALLOC(free)
inline void *reallocarrayf(void *p, size_t nmemb, size_t size);


inline void *
reallocarrayf(void *p, size_t nmemb, size_t size)
{
	void  *q;

	q = reallocarray(p, nmemb, size);

	/* realloc(p, 0) is equivalent to free(p);  avoid double free.  */
	if (q == NULL && nmemb != 0 && size != 0)
		free(p);
	return q;
}


#endif  // include guard
