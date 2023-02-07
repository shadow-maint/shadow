/*
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int get_pid (const char *pidstr, pid_t *pid)
{
	long long int val;
	char *endptr;

	errno = 0;
	val = strtoll (pidstr, &endptr, 10);
	if (   ('\0' == *pidstr)
	    || ('\0' != *endptr)
	    || (ERANGE == errno)
	    || (/*@+longintegral@*/val != (pid_t)val)/*@=longintegral@*/) {
		return 0;
	}

	*pid = val;
	return 1;
}

/*
 * If use passed in fd:4 as an argument, then return the
 * value '4', the fd to use.
 */
int get_pidfd_from_fd(const char *pidfdstr)
{
	long long int val;
	char *endptr;

	errno = 0;
	val = strtoll (pidfdstr, &endptr, 10);
	if (   ('\0' == *pidfdstr)
	    || ('\0' != *endptr)
	    || (ERANGE == errno)
	    || (/*@+longintegral@*/val != (pid_t)val)/*@=longintegral@*/) {
		return 0;
	}

	return (int)val;
}

int open_pidfd(const char *pidstr)
{
	int proc_dir_fd;
	int written;
	char proc_dir_name[32];
	pid_t target;

	if (get_pid(pidstr, &target) == 0)
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
