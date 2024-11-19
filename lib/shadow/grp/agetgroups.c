// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "shadow/grp/agetgroups.h"

#include <stddef.h>
#include <sys/types.h>


extern inline gid_t *agetgroups(size_t *ngids);
