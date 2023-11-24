/*
 * SPDX-FileCopyrightText: 2022-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#include <stddef.h>

#ident "$Id$"

#include "string/ustr2stp.h"


extern inline char *ustr2stp(char *restrict dst, const char *restrict src,
    size_t ssize);
