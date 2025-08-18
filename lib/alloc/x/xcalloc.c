// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1998, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2006, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2008     , Nicolas François
// SPDX-FileCopyrightText: 2023-2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "alloc/x/xcalloc.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "io/fprintf/fprinte.h"
#include "shadowlog.h"


void *
xcalloc(size_t nmemb, size_t size)
{
	void  *p;

	p = calloc(nmemb, size);
	if (p == NULL)
		goto x;

	return p;

x:
	fprinte(log_get_logfd(), "%s", log_get_progname());
	exit(13);
}
