
#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include "groupio.h"

struct group *__gr_dup (const struct group *grent)
{
	struct group *gr;
	int i;

	if (!(gr = (struct group *) malloc (sizeof *gr)))
		return NULL;
	*gr = *grent;
	if (!(gr->gr_name = strdup (grent->gr_name)))
		return NULL;
	if (!(gr->gr_passwd = strdup (grent->gr_passwd)))
		return NULL;

	for (i = 0; grent->gr_mem[i]; i++);
	gr->gr_mem = (char **) malloc ((i + 1) * sizeof (char *));
	if (!gr->gr_mem)
		return NULL;
	for (i = 0; grent->gr_mem[i]; i++) {
		gr->gr_mem[i] = strdup (grent->gr_mem[i]);
		if (!gr->gr_mem[i])
			return NULL;
	}
	gr->gr_mem[i] = NULL;
	return gr;
}

