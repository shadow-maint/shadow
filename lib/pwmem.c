
#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
#include <stdio.h>
#include "pwio.h"

struct passwd *__pw_dup (const struct passwd *pwent)
{
	struct passwd *pw;

	if (!(pw = (struct passwd *) malloc (sizeof *pw)))
		return NULL;
	*pw = *pwent;
	if (!(pw->pw_name = strdup (pwent->pw_name)))
		return NULL;
	if (!(pw->pw_passwd = strdup (pwent->pw_passwd)))
		return NULL;
	if (!(pw->pw_gecos = strdup (pwent->pw_gecos)))
		return NULL;
	if (!(pw->pw_dir = strdup (pwent->pw_dir)))
		return NULL;
	if (!(pw->pw_shell = strdup (pwent->pw_shell)))
		return NULL;
	return pw;
}

