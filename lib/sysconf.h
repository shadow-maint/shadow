// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-FileCopyrightText: 2026, Tobias Stoeckmann <tobias@stoeckmann.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef _SYSCONF_H_
#define _SYSCONF_H_


#include "config.h"

#include <stddef.h>


extern size_t login_name_max_size(void);
extern size_t ngroups_max_size(void);

#endif  // include guard
