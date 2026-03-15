// SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_STRING_MEMSET_MEMZERO_H_
#define SHADOW_INCLUDE_LIB_STRING_MEMSET_MEMZERO_H_


#include "config.h"

#include <stddef.h>
#include <string.h>

#include "sizeof.h"
#include "typetraits.h"


// memzero_a - memory zero (explicit) array
#define memzero_a(arr)  memzero(arr, sizeof_a(arr))

// memzero - memory zero (explicit)
#define memzero(p, ...)  ((QVoid_of(p) *) memzero_(p, __VA_ARGS__))
// strzero - string zero (explicit)
#define strzero(s)                                                    \
({                                                                    \
	VQChar_of(s)  *s_ = s;                                        \
	                                                              \
	memzero(s_, strlen(s_));                                      \
})


inline void *memzero_(volatile void *ptr, size_t size);


inline void *
memzero_(volatile void *ptr, size_t size)
{
	explicit_bzero((void *) ptr, size);
	return (void *) ptr;
}


#endif  // include guard
