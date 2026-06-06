// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-FileCopyrightText: 2026, Tobias Stoeckmann <tobias@stoeckmann.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <limits.h>
#include <unistd.h>


#ifndef  NGROUPS_MAX
# define NGROUPS_MAX  65536
#endif


size_t
ngroups_max_size(void)
{
	long  conf;

	conf = sysconf(_SC_NGROUPS_MAX);
	if (conf == -1)
		conf = NGROUPS_MAX;

	return conf;
}
