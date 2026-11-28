// SPDX-FileCopyrightText: 2026, Serge Hallyn <serge@hallyn.com>
// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "fs/open/open.h"

#include <fcntl.h>
#include <linux/openat2.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>



// openat2(2)
#define my_openat2(d, p, h)  my_openat2__(d, p, h, sizeof(struct open_how))


static int my_openat2__(int dirfd, const char *path, struct open_how *how,
    size_t size);


/*
 * openat(2), but ensuring that there are no bind mounts or symlinks
 * redirecting the path walk.
 * The arguments are the same as for openat(2).
 */
int
samedev_openat(int dirfd, const char *path, int flags)
{
	struct open_how  how = {
		.flags   = flags,
		.resolve = RESOLVE_NO_XDEV | RESOLVE_NO_SYMLINKS |
			RESOLVE_NO_MAGICLINKS,
	};

	return my_openat2(dirfd, path, &how);
}


static int
my_openat2__(int dirfd, const char *path, struct open_how *how, size_t size)
{
	return syscall(SYS_openat2, dirfd, path, how, size);
}
