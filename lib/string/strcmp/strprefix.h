// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_STRCMP_STRPREFIX_H_
#define SHADOW_INCLUDE_LIB_STRING_STRCMP_STRPREFIX_H_


#include <config.h>

#include <stddef.h>
#include <string.h>

#include "attr.h"
#include "cast.h"


// string prefix
#define strprefix(s, prefix)                                          \
({                                                                    \
	const char  *p_;                                              \
                                                                      \
	p_ = strprefix_(s, prefix);                                   \
                                                                      \
	_Generic(s,                                                   \
		const char *:                     p_,                 \
		const void *:                     p_,                 \
		char *:        const_cast(char *, p_),                \
		void *:        const_cast(char *, p_)                 \
	);                                                            \
})


ATTR_STRING(1)
ATTR_STRING(2)
inline const char *strprefix_(const char *s, const char *prefix);


/*
 * Return NULL if s does not start with prefix.
 * Return `s + strlen(prefix)` if s starts with prefix.
 */
inline const char *
strprefix_(const char *s, const char *prefix)
{
	if (strncmp(s, prefix, strlen(prefix)) != 0)
		return NULL;

	return s + strlen(prefix);
}


#endif  // include guard
