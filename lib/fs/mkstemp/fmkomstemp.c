// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "fs/mkstemp/fmkomstemp.h"

#include <stdio.h>
#include <sys/types.h>


extern inline FILE *fmkomstemp(char *template, unsigned int flags, mode_t m);
