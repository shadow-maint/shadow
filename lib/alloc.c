/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 * SPDX-FileCopyrightText: 2023       , Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include "alloc.h"

#include <stddef.h>


extern inline void *mallocarray(size_t nmemb, size_t size);
extern inline void *reallocarrayf(void *p, size_t nmemb, size_t size);
