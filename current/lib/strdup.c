#include <config.h>
#include "defines.h"
#include "rcsid.h"
RCSID("$Id: strdup.c,v 1.2 1997/12/07 23:26:59 marekm Exp $")

extern char *malloc();

char *
strdup(const char *str)
{
	char *s = malloc(strlen(str) + 1);

	if (s)
		strcpy(s, str);
	return s;
}
