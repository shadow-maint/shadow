// SPDX-FileCopyrightText: 2009, Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defines.h"
#include "atoi/getnum.h"
#include "defines.h"
#include "prototypes.h"
#include "string/sprintf/snprintf.h"


/*
 * If use passed in fd:4 as an argument, then return the
 * value '4', the fd to use.
 * On error, return -1.
 */
int get_pidfd_from_fd(const char *pidfdstr)
{
	int          pidfd;
	struct stat  st;
	dev_t proc_st_dev, proc_st_rdev;

	if (get_fd(pidfdstr, &pidfd) == -1)
		return -1;

	if (stat("/proc/self/uid_map", &st) < 0) {
		return -1;
	}

	proc_st_dev = st.st_dev;
	proc_st_rdev = st.st_rdev;

	if (fstat(pidfd, &st) < 0) {
		return -1;
	}

	if (st.st_dev != proc_st_dev || st.st_rdev != proc_st_rdev) {
		return -1;
	}

	return pidfd;
}

int open_pidfd(const char *pidstr)
{
	int    proc_dir_fd;
	char   proc_dir_name[PATH_MAX];
	pid_t  target;

	if (get_pid(pidstr, &target) == -1)
		return -ENOENT;

	if (SNPRINTF(proc_dir_name, "/proc/%d/", target) == -1) {
		fprintf(stderr, "snprintf of proc path failed for %d: %s\n",
			target, strerror(errno));
		return -EINVAL;
	}

	proc_dir_fd = open(proc_dir_name, O_DIRECTORY);
	if (proc_dir_fd < 0) {
		fprintf(stderr, _("Could not open proc directory for target %d: %s\n"),
			target, strerror(errno));
		return -EINVAL;
	}
	return proc_dir_fd;
}
