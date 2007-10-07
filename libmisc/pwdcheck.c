#include <config.h>

#ident "$Id: pwdcheck.c,v 1.9 2005/10/19 15:21:07 kloczek Exp $"

#include <pwd.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include "pwauth.h"
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif
#ifdef USE_PAM
#include "pam_defs.h"
#endif
#define WRONGPWD2	"incorrect password for `%s'"
void passwd_check (const char *user, const char *passwd, const char *progname)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retcode;

	if (pam_start (progname, user, &conv, &pamh)) {
	      bailout:
		SYSLOG ((LOG_WARN, WRONGPWD2, user));
		sleep (1);
		fprintf (stderr, _("Incorrect password for %s.\n"), user);
		exit (1);
	}
	if (pam_authenticate (pamh, 0))
		goto bailout;

	retcode = pam_acct_mgmt (pamh, 0);
	if (retcode == PAM_NEW_AUTHTOK_REQD)
		retcode = pam_chauthtok (pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
	if (retcode)
		goto bailout;

	if (pam_setcred (pamh, 0))
		goto bailout;

	/* no need to establish a session; this isn't a session-oriented
	 * activity... */

#else				/* !USE_PAM */

	struct spwd *sp;

	if ((sp = getspnam (user)))
		passwd = sp->sp_pwdp;
	endspent ();
	if (pw_auth (passwd, user, PW_LOGIN, (char *) 0) != 0) {
		SYSLOG ((LOG_WARN, WRONGPWD2, user));
		sleep (1);
		fprintf (stderr, _("Incorrect password for %s.\n"), user);
		exit (1);
	}
#endif				/* !USE_PAM */
}
