/*
 * SPDX-FileCopyrightText:  2023, Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_MALLOC_H_
#define SHADOW_INCLUDE_LIB_MALLOC_H_


#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "attr.h"
#include "defines.h"
#include "x.h"


#define CALLOC(n, type)   ((type *) calloc(n, sizeof(type)))
#define XCALLOC(n, type)  ((type *) x(calloc(n, sizeof(type))))
#define MALLOC(n, type)   ((type *) mallocarray(n, sizeof(type)))
#define XMALLOC(n, type)  ((type *) x(mallocarray(n, sizeof(type))))

#define REALLOC(ptr, n, type)                                                 \
({                                                                            \
	__auto_type  p_ = (ptr);                                              \
                                                                              \
	static_assert(__builtin_types_compatible_p(typeof(p_), type *), "");  \
                                                                              \
	(type *) reallocarray(p_, n, sizeof(type));                           \
})

#define REALLOCF(ptr, n, type)                                                \
({                                                                            \
	__auto_type  p_ = (ptr);                                              \
                                                                              \
	static_assert(__builtin_types_compatible_p(typeof(p_), type *), "");  \
                                                                              \
	(type *) reallocarrayf(p_, n, sizeof(type));                          \
})

#define XREALLOC(ptr, n, type)                                                \
({                                                                            \
	__auto_type  p_ = (ptr);                                              \
                                                                              \
	static_assert(__builtin_types_compatible_p(typeof(p_), type *), "");  \
                                                                              \
	(type *) x(reallocarray(p_, n, sizeof(type)));                        \
})


inline void *mallocarray(size_t nmemb, size_t size);
ATTR_MALLOC(free)
inline void *reallocarrayf(void *p, size_t nmemb, size_t size);


inline void *
mallocarray(size_t nmemb, size_t size)
{
	return reallocarray(NULL, nmemb, size);
}


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
