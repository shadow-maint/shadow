// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2006, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008     , Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "alloc/x/xrealloc.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "alloc/reallocf.h"
#include "defines.h"
#include "shadowlog.h"


void *
xreallocarray(void *p, size_t nmemb, size_t size)
{
	p = reallocarrayf(p, nmemb, size);
	if (p == NULL)
		goto x;

	return p;

x:
	fprintf(log_get_logfd(), _("%s: %s\n"),
	        log_get_progname(), strerror(errno));
	exit(13);
}
