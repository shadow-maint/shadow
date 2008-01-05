#include <config.h>

#ifdef USE_PAM

#ident "$Id$"


/*
 * Change the user's password using PAM.  Requires libpam and libpam_misc
 * (for misc_conv).  Note: libpam_misc is probably Linux-PAM specific,
 * so you may have to port it if you want to use this code on non-Linux
 * systems with PAM (such as Solaris 2.6).  --marekm
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "defines.h"
#include "pam_defs.h"

/* local function prototypes */
static void do_pam_passwd (const char *user, int silent, int change_expired);

static void do_pam_passwd (const char *user, int silent, int change_expired)
{
	pam_handle_t *pamh = NULL;
	int flags = 0, ret;

	if (silent)
		flags |= PAM_SILENT;
	if (change_expired)
		flags |= PAM_CHANGE_EXPIRED_AUTHTOK;

	ret = pam_start ("passwd", user, &conv, &pamh);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr,
			 _("passwd: pam_start() failed, error %d\n"), ret);
		exit (10);	/* XXX */
	}

	ret = pam_chauthtok (pamh, flags);
	if (ret != PAM_SUCCESS) {
		fprintf (stderr, _("passwd: %s\n"), pam_strerror (pamh, ret));
		pam_end (pamh, ret);
		exit (10);	/* XXX */
	}

	fputs (_("passwd: password updated successfully\n"), stderr);
	pam_end (pamh, PAM_SUCCESS);
}
#else				/* !USE_PAM */
extern int errno;		/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */
