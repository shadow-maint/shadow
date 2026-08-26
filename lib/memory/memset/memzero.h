// SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMSET_MEMZERO_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMSET_MEMZERO_H_


#include "config.h"

#include <memory.h>
#include <stddef.h>
#include <strings.h>

#include "sizeof.h"


// memzero_a - memory zero (explicit) array
#define memzero_a(arr)  memzero(arr, sizeof_a(arr))


inline void *memzero(void *ptr, size_t size);


// memzero - memory zero (explicit)
inline void *
memzero(void *ptr, size_t size)
{
#if defined(HAVE_MEMSET_EXPLICIT)
	memset_explicit(ptr, 0, size);
#elif defined(HAVE_EXPLICIT_BZERO)
	explicit_bzero(ptr, size);
#else
	bzero(ptr, size);
	__asm__ __volatile__ ("" : : "r"(ptr) : "memory");
#endif
	return ptr;
}


#endif  // include guard
