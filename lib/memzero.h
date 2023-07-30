/*
 * SPDX-FileCopyrightText: 2022-2023, Christian Göttsche <cgzones@googlemail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIBMISC_MEMZERO_H_
#define SHADOW_INCLUDE_LIBMISC_MEMZERO_H_


#include <config.h>

#include <stddef.h>
#include <string.h>
#include <strings.h>


#ifdef HAVE_MEMSET_EXPLICIT
# define memzero(ptr, size) memset_explicit((ptr), 0, (size))
#elif defined HAVE_EXPLICIT_BZERO	/* !HAVE_MEMSET_S */
# define memzero(ptr, size) explicit_bzero((ptr), (size))
#else					/* !HAVE_MEMSET_S && HAVE_EXPLICIT_BZERO */
static inline void memzero(void *ptr, size_t size)
{
	ptr = memset(ptr, '\0', size);
	__asm__ __volatile__ ("" : : "r"(ptr) : "memory");
}
#endif					/* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO */

#define strzero(s) memzero(s, strlen(s))	/* warning: evaluates twice */


#endif  // include guard
