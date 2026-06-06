// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-FileCopyrightText: 2026, Tobias Stoeckmann <tobias@stoeckmann.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef _SYSCONF_H_
#define _SYSCONF_H_


#include "config.h"

#include <limits.h>
#include <stddef.h>


#ifndef  LOGIN_NAME_MAX
# define LOGIN_NAME_MAX  256
#endif


extern size_t ngroups_max_size(void);

#endif  // include guard
