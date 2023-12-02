/*
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "atoi/strtoi.h"
#include "defines.h"
#include "prototypes.h"
#include "types.h"


int
get_pid(const char *pidstr, pid_t *pid)
{
	int    status;
	pid_t  val;

	val = strtoi(pidstr, NULL, 10, 1, type_max(pid_t), &status);
	if (status != 0)
		return -1;

	*pid = val;
	return 0;
}

/*
 * If use passed in fd:4 as an argument, then return the
 * value '4', the fd to use.
 * On error, return -1.
 */
int get_pidfd_from_fd(const char *pidfdstr)
{
	int          val, status;
	dev_t        proc_st_dev, proc_st_rdev;
	struct stat  st;

	val = strtoi(pidfdstr, NULL, 10, 0, INT_MAX, &status);
	if (status != 0)
		return -1;

	if (stat("/proc/self/uid_map", &st) < 0) {
		return -1;
	}

	proc_st_dev = st.st_dev;
	proc_st_rdev = st.st_rdev;

	if (fstat(val, &st) < 0) {
		return -1;
	}

	if (st.st_dev != proc_st_dev || st.st_rdev != proc_st_rdev) {
		return -1;
	}

	return (int)val;
}

int open_pidfd(const char *pidstr)
{
	int proc_dir_fd;
	int written;
	char proc_dir_name[32];
	pid_t target;

	if (get_pid(pidstr, &target) == -1)
		return -ENOENT;

	/* max string length is 6 + 10 + 1 + 1 = 18, allocate 32 bytes */
	written = snprintf(proc_dir_name, sizeof(proc_dir_name), "/proc/%u/",
		target);
	if ((written <= 0) || ((size_t)written >= sizeof(proc_dir_name))) {
		fprintf(stderr, "snprintf of proc path failed for %u: %s\n",
			target, strerror(errno));
		return -EINVAL;
	}

	proc_dir_fd = open(proc_dir_name, O_DIRECTORY);
	if (proc_dir_fd < 0) {
		fprintf(stderr, _("Could not open proc directory for target %u: %s\n"),
			target, strerror(errno));
		return -EINVAL;
	}
	return proc_dir_fd;
}
