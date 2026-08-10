/*
 * Copyright 1990, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#include <string.h>
#include "config.h"

#ifndef lint
static	char	sccsid[] = "@(#)encrypt.c	3.5	07:45:28	22 Apr 1993";
#endif

extern	char	*crypt();

char *
pw_encrypt (clear, salt)
char	*clear;
char	*salt;
{
#ifdef	SW_CRYPT
	static	char	cipher[128];
#else
	static	char	cipher[32];
#endif
	static	int	count;
	char	newsalt[2];
	char	*cp;
	long	now;

	/*
	 * See if a new salt is needed and get a few random
	 * bits of information.  The amount of randomness is
	 * probably not all that crucial since the salt only
	 * serves to thwart a dictionary attack.
	 */

	if (salt == (char *) 0) {
		now = time ((long *) 0) + count++;
		now ^= clock ();
		now ^= getpid ();
		now = ((now >> 12) ^ (now)) & 07777;
		newsalt[0] = i64c ((now >> 6) & 077);
		newsalt[1] = i64c (now & 077);
		salt = newsalt;
	}
#ifdef	SW_CRYPT
	/*
	 * Copy over the salt.  It is always the first two
	 * characters of the string.
	 */

	cipher[0] = salt[0];
	cipher[1] = salt[1];
	cipher[2] = '\0';

	/*
	 * Loop up to ten times on the cleartext password.
	 * This is because the input limit for passwords is
	 * 80 characters.
	 *
	 * The initial salt is that provided by the user, or the
	 * one generated above.  The subsequent salts are gotten
	 * from the first two characters of the previous encrypted
	 * block of characters.
	 */

	for (count = 0;count < 10;count++) {
		cp = crypt (clear, salt);
		strcat (cipher, cp + 2);
		salt = cipher + 11 * count + 2;

		if (strlen (clear) > 8)
			clear += 8;
		else
			break;
	}
#else
	cp = crypt (clear, salt);
	strcpy (cipher, cp);

#ifdef	DOUBLESIZE
	if (strlen (clear) > 8) {
		cp = crypt (clear + 8, salt);
		strcat (cipher, cp + 2);
	}
#endif	/* DOUBLESIZE */
#endif	/* SW_CRYPT */
	return cipher;
}
