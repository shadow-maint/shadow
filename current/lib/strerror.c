#include <config.h>
#include <errno.h>
#include "defines.h"
#include "rcsid.h"
RCSID("$Id: strerror.c,v 1.3 1998/12/28 20:34:39 marekm Exp $")

#include <stdio.h>

extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror(int err)
{
	static char unknown[80];

	if (err >= 0 && err < sys_nerr)
		return sys_errlist[err];

	snprintf(unknown, sizeof unknown, _("Unknown error %d"), err);
	errno = EINVAL;
	return unknown;
}
