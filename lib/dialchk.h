/* $Id: dialchk.h,v 1.1 1997/12/07 23:26:49 marekm Exp $ */
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
extern int dialcheck P_((const char *tty, const char *sh));

#endif
