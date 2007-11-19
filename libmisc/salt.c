/*
 * salt.c - generate a random salt string for crypt()
 *
 * Written by Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>,
 * public domain.
 *
 * l64a was Written by J.T. Conklin <jtc@netbsd.org>. Public domain.
 */

#include <config.h>

#ident "$Id$"

#include <sys/time.h>
#include <stdlib.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

#ifndef HAVE_L64A
char *l64a(long value)
{
	static char buf[8];
	char *s = buf;
	int digit;
	int i;

	if (value < 0) {
		errno = EINVAL;
		return(NULL);
	}

	for (i = 0; value != 0 && i < 6; i++) {
		digit = value & 0x3f;

		if (digit < 2) 
			*s = digit + '.';
		else if (digit < 12)
			*s = digit + '0' - 2;
		else if (digit < 38)
			*s = digit + 'A' - 12;
		else
			*s = digit + 'a' - 38;

		value >>= 6;
		s++;
	}

	*s = '\0';

	return(buf);
}
#endif /* !HAVE_L64A */

/*
 * Generate 8 base64 ASCII characters of random salt.  If MD5_CRYPT_ENAB
 * in /etc/login.defs is "yes", the salt string will be prefixed by "$1$"
 * (magic) and pw_encrypt() will execute the MD5-based FreeBSD-compatible
 * version of crypt() instead of the standard one.
 */

#define MAGNUM(array,ch)	(array)[0]= (array)[2] = '$',\
				(array)[1]=(ch),\
				(array)[2]='\0'

char *crypt_make_salt (void)
{
	struct timeval tv;
	static char result[40];
	int max_salt_len = 8;
	char *method;

	result[0] = '\0';

#ifndef USE_PAM
#ifdef ENCRYPTMETHOD_SELECT
	if ((method = getdef_str ("ENCRYPT_METHOD")) == NULL) {
#endif
		if (getdef_bool ("MD5_CRYPT_ENAB")) {
			MAGNUM(result,'1');
			max_salt_len = 11;
		}
#ifdef ENCRYPTMETHOD_SELECT
	} else {
		if (!strncmp (method, "MD5", 3)) {
			MAGNUM(result, '1');
			max_salt_len = 11;
		} else if (!strncmp (method, "SHA256", 6)) {
			MAGNUM(result, '5');
			max_salt_len = 11; /* XXX: should not be fixed */
		} else if (!strncmp (method, "SHA512", 6)) {
			MAGNUM(result, '6');
			max_salt_len = 11; /* XXX: should not be fixed */
		} else if (0 != strncmp (method, "DES", 3)) {
			fprintf (stderr,
				 _("Invalid ENCRYPT_METHOD value: '%s'.\n"
				   "Defaulting to DES.\n"),
				 method);
			result[0] = '\0';
		}
	}
#endif				/* ENCRYPTMETHOD_SELECT */
#endif				/* USE_PAM */
	/*
	 * Generate 8 chars of salt, the old crypt() will use only first 2.
	 */
	gettimeofday (&tv, (struct timezone *) 0);
	strcat (result, l64a (tv.tv_usec));
	strcat (result, l64a (tv.tv_sec + getpid () + clock ()));

	if (strlen (result) > max_salt_len)	/* magic+salt */
		result[max_salt_len] = '\0';

	return result;
}
