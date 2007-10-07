/*
 * salt.c - generate a random salt string for crypt()
 *
 * Written by Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>,
 * public domain.
 */

#include <config.h>

#include "rcsid.h"
RCSID ("$Id: salt.c,v 1.7 2005/04/06 00:21:37 kloczek Exp $")
#include <sys/time.h>
#include <stdlib.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

/*
 * Generate 8 base64 ASCII characters of random salt.  If MD5_CRYPT_ENAB
 * in /etc/login.defs is "yes", the salt string will be prefixed by "$1$"
 * (magic) and pw_encrypt() will execute the MD5-based FreeBSD-compatible
 * version of crypt() instead of the standard one.
 */
char *crypt_make_salt (void)
{
	struct timeval tv;
	static char result[40];

	result[0] = '\0';
	if (getdef_bool ("MD5_CRYPT_ENAB")) {
		strcpy (result, "$1$");	/* magic for the new MD5 crypt() */
	}

	/*
	 * Generate 8 chars of salt, the old crypt() will use only first 2.
	 */
	gettimeofday (&tv, (struct timezone *) 0);
	strcat (result, l64a (tv.tv_usec));
	strcat (result, l64a (tv.tv_sec + getpid () + clock ()));

	if (strlen (result) > 3 + 8)	/* magic+salt */
		result[11] = '\0';

	return result;
}
