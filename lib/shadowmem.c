
#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif
#include <stdio.h>
#include "shadowio.h"

struct spwd *__spw_dup (const struct spwd *spent)
{
	struct spwd *sp;

	if (!(sp = (struct spwd *) malloc (sizeof *sp)))
		return NULL;
	*sp = *spent;
	if (!(sp->sp_namp = strdup (spent->sp_namp)))
		return NULL;
	if (!(sp->sp_pwdp = strdup (spent->sp_pwdp)))
		return NULL;
	return sp;
}

