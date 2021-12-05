/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1998, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2006, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Replacements for malloc and strdup with error checking.  Too trivial
   to be worth copyrighting :-).  I did that because a lot of code used
   malloc and strdup without checking for NULL pointer, and I like some
   message better than a core dump...  --marekm
   
   Yeh, but.  Remember that bailing out might leave the system in some
   bizarre state.  You really want to put in error checking, then add
   some back-out failure recovery code. -- jfh */

#include <config.h>

#ident "$Id$"

#include <stdio.h>
#include <errno.h>
#include "defines.h"
#include "prototypes.h"

/*@maynotreturn@*/ /*@only@*//*@out@*//*@notnull@*/char *xmalloc (size_t size)
{
	char *ptr;

	ptr = (char *) malloc (size);
	if (NULL == ptr) {
		(void) fprintf (shadow_logfd,
		                _("%s: failed to allocate memory: %s\n"),
		                Prog, strerror (errno));
		exit (13);
	}
	return ptr;
}

/*@maynotreturn@*/ /*@only@*//*@notnull@*/char *xstrdup (const char *str)
{
	return strcpy (xmalloc (strlen (str) + 1), str);
}

void xfree(void *ap)
{
	if (ap) {
		free(ap);
	}
}
