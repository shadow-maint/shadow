// SPDX-FileCopyrightText: 1989-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1999, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2001-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008-2009, Nicolas François
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include <fcntl.h>
#include <unistd.h>
#include <utmpx.h>

#include "prototypes.h"


inline void updwtmpx(const char *filename, const struct utmpx *ut);


#if !defined(HAVE_UPDWTMPX)
// updwtmpx(3) - update wtmp(5) struct-utmpx
inline void
updwtmpx(const char *filename, const struct utmpx *ut)
{
	int fd;

	fd = open (filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write_full(fd, ut, sizeof(*ut));
		close (fd);
	}
}
#endif
