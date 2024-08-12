// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <stddef.h>

#include "string/memset/memzero.h"


extern inline void *memzero(void *ptr, size_t size);
extern inline char *strzero(char *s);
