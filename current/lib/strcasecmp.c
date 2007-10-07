#include <config.h>
#include "defines.h"
#include <ctype.h>

#include "rcsid.h"
RCSID("$Id: strcasecmp.c,v 1.1 1999/07/09 18:02:43 marekm Exp $")

/*
 * strcasecmp - compare strings, ignoring case
 */

char *
strcasecmp(const char *s1, const char *s2)
{
	int ret;

	for (;;) {
		ret = tolower(*s1) - tolower(*s2);
		if (ret || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}
	return ret;
}
