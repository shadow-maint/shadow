// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "fs/readlink/readlinknul.h"

#include <stddef.h>
#include <sys/types.h>


extern inline ssize_t readlinknul(const char *restrict link, char *restrict buf,
    size_t size);
