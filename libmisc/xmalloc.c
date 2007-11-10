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
#include "defines.h"
char *xmalloc (size_t size)
{
	char *ptr;

	ptr = (char *) malloc (size);
	if (!ptr && size) {
		fprintf (stderr, _("malloc(%d) failed\n"), (int) size);
		exit (13);
	}
	return ptr;
}

char *xstrdup (const char *str)
{
	return strcpy (xmalloc (strlen (str) + 1), str);
}
