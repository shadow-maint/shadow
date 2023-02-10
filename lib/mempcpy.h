/*
 * SPDX-FileCopyrightText:  2023, Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIB_MEMPCPY_H_
#define SHADOW_INCLUDE_LIB_MEMPCPY_H_


#include <config.h>

#if !defined(HAVE_MEMPCPY)

#include <stddef.h>
#include <string.h>


inline void *mempcpy(void *restrict dst, const void *restrict src, size_t n);


inline void *
mempcpy(void *restrict dst, const void *restrict src, size_t n)
{
	return memcpy(dst, src, n) + n;
}


#endif  // !HAVE_MEMPCPY
#endif  // include guard
