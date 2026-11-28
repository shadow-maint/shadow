// SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
// SPDX-FileCopyrightText: 2023-2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_MEMORY_MEMSET_MEMZERO_H_
#define SHADOW_INCLUDE_LIB_MEMORY_MEMSET_MEMZERO_H_


#include "config.h"

#include <memory.h>
#include <stddef.h>

#include "sizeof.h"
#include "typetraits.h"


// memzero_explicit_a - memory zero explicit array
#define memzero_explicit_a(arr)  memzero_explicit(arr, sizeof_a(arr))

// memzero_explicit - memory zero explicit
#define memzero_explicit(p, ...)  ((QVoid_of(p) *) memzero_explicit_(p, __VA_ARGS__))


inline volatile void *memzero_explicit_(volatile void *ptr, size_t size);


inline volatile void *
memzero_explicit_(volatile void *ptr, size_t size)
{
	explicit_bzero((void *) ptr, size);
	return ptr;
}


#endif  // include guard
