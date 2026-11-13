// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defines.h"
#include "atoi/getnum.h"
#include "defines.h"
#include "io/fprintf.h"
#include "prototypes.h"
#include "string/sprintf/stprintf.h"


int open_pidfd(const char *pidstr)
{
	int    proc_dir_fd;
	char   proc_dir_name[PATH_MAX];
	pid_t  target;

	if (get_pid(pidstr, &target) == -1)
		return -ENOENT;

	if (stprintf_a(proc_dir_name, "/proc/%d/", target) == -1) {
		eprinte("snprintf of proc path failed for %d", target);
		return -EINVAL;
	}

	proc_dir_fd = open(proc_dir_name, O_DIRECTORY);
	if (proc_dir_fd < 0) {
		eprinte(_("Could not open proc directory for target %d"), target);
		return -EINVAL;
	}
	return proc_dir_fd;
}
