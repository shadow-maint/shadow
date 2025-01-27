// SPDX-FileCopyrightText: 1990-1994, Julianne Frances Haugh
// SPDX-FileCopyrightText: 1996-1997, Marek Michałkiewicz
// SPDX-FileCopyrightText: 2003-2005, Tomasz Kłoczko
// SPDX-FileCopyrightText: 2024, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause

/*
 * basename.c - not worth copyrighting :-).  Some versions of Linux libc
 * already have basename(), other versions don't.  To avoid confusion,
 * we will not use the function from libc and use a different name here.
 * --marekm
 */

#include <config.h>

#include <stddef.h>
#include <stdlib.h>

#include "prototypes.h"
#include "string/strspn/stprcspn.h"


/*@observer@*/const char *
Basename(const char *str)
{
	if (str == NULL) {
		abort ();
	}

	return stprcspn(str, "/");
}
