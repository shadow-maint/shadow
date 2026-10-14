// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "fs/copy/fcopy.h"

#include <stdio.h>


extern inline int fcopy(FILE *dst, FILE *src);
