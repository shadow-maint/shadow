/* $Id: dialchk.h,v 1.2 2000/08/26 18:27:17 marekm Exp $ */
#ifndef _DIALCHK_H_
#define _DIALCHK_H_

#include "defines.h"

/*
 * Check for dialup password
 *
 *	dialcheck tests to see if tty is listed as being a dialup
 *	line.  If so, a dialup password may be required if the shell
 *	is listed as one which requires a second password.
 */
extern int dialcheck(const char *, const char *);

#endif
