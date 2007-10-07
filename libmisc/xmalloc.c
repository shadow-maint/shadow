/* Replacements for malloc and strdup with error checking.  Too trivial
   to be worth copyrighting :-).  I did that because a lot of code used
   malloc and strdup without checking for NULL pointer, and I like some
   message better than a core dump...  --marekm
   
   Yeh, but.  Remember that bailing out might leave the system in some
   bizarre state.  You really want to put in error checking, then add
   some back-out failure recovery code. -- jfh */

#include <config.h>

#include "rcsid.h"
RCSID ("$Id: xmalloc.c,v 1.5 2004/05/06 21:31:33 kloczek Exp $")
#include <stdio.h>
#include "defines.h"

char *xmalloc (size_t size)
{
	char *ptr;

	ptr = malloc (size);
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
