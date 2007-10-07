/*
 * basename.c - not worth copyrighting :-).  Some versions of Linux libc
 * already have basename(), other versions don't.  To avoid confusion,
 * we will not use the function from libc and use a different name here.
 * --marekm
 */

#include <config.h>

#include "rcsid.h"
RCSID("$Id: basename.c,v 1.2 1997/12/07 23:27:00 marekm Exp $")

#include "defines.h"
#include "prototypes.h"

char *
Basename(char *str)
{
	char *cp = strrchr(str, '/');

	return cp ? cp+1 : str;
}
