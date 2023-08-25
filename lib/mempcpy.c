/*
 * SPDX-FileCopyrightText:  2023, Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */


#include <config.h>

#if !defined(HAVE_MEMPCPY)

#ident "$Id$"

#include "mempcpy.h"

#include <stddef.h>


extern inline void *mempcpy(void *restrict dst, const void *restrict src,
    size_t n);


#endif
