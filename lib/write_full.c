/*
 * SPDX-FileCopyrightText:  2023, Christian Göttsche <cgzones@googlemail.com>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */


#include <config.h>

#ident "$Id$"

#include "prototypes.h"

#include <errno.h>
#include <unistd.h>


/*
 * write_full - write entire buffer
 *
 * Write up to count bytes from the buffer starting at buf to the
 * file referred to by the file descriptor fd.
 * Retry in case of a short write.
 *
 * Returns the number of bytes written on success, -1 on error.
 */
ssize_t write_full(int fd, const void *buf, size_t count) {
	ssize_t written = 0;

	while (count > 0) {
		ssize_t res;

		res = write(fd, buf, count);
		if (res < 0) {
			if (errno == EINTR) {
				continue;
			}

			return res;
		}

		if (res == 0) {
			break;
		}

		written += res;
		buf = (const unsigned char*)buf + res;
		count -= res;
	}

	return written;
}
