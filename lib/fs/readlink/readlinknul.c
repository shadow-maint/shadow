// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "fs/readlink/readlinknul.h"

#include <sys/types.h>


extern inline ssize_t readlinknul(ssize_t size;
    const char *restrict link, char buf[restrict size], ssize_t size);
