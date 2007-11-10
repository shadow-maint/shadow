/*
 * basename.c - not worth copyrighting :-).  Some versions of Linux libc
 * already have basename(), other versions don't.  To avoid confusion,
 * we will not use the function from libc and use a different name here.
 * --marekm
 */

#include <config.h>

#ident "$Id$"

#include "defines.h"
#include "prototypes.h"
char *Basename (char *str)
{
	char *cp = strrchr (str, '/');

	return cp ? cp + 1 : str;
}
