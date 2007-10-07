/*
 * Copyright 1990 - 1993, Julianne Frances Haugh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include "rcsid.h"
RCSID("$Id: encrypt.c,v 1.6 1999/03/07 19:14:35 marekm Exp $")

#include "prototypes.h"
#include "defines.h"

extern	char	*crypt();
extern char *libshadow_md5_crypt P_((const char *, const char *));

char *
pw_encrypt(const char *clear, const char *salt)
{
	static	char	cipher[128];
	char	*cp;
#ifdef SW_CRYPT
	static	int	count;
#endif

#ifdef MD5_CRYPT
	/*
	 * If the salt string from the password file or from crypt_make_salt()
	 * begins with the magic string, use the new algorithm.
	 */
	if (strncmp(salt, "$1$", 3) == 0)
		return libshadow_md5_crypt(clear, salt);
#endif

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
		cp = crypt(clear, salt);
		if (!cp) {
			perror("crypt");
			exit(1);
		}
		if (strlen(cp) != 13)
			return cp;
		strcat(cipher, cp + 2);
		salt = cipher + 11 * count + 2;

		if (strlen(clear) > 8)
			clear += 8;
		else
			break;
	}
#else
	cp = crypt(clear, salt);
	if (!cp) {
		/*
		 * Single Unix Spec: crypt() may return a null pointer,
		 * and set errno to indicate an error.  The caller doesn't
		 * expect us to return NULL, so...
		 */
		perror("crypt");
		exit(1);
	}
	if (strlen(cp) != 13)
		return cp;  /* nonstandard crypt() in libc, better bail out */
	strcpy(cipher, cp);

#ifdef	DOUBLESIZE
	if (strlen (clear) > 8) {
		cp = crypt(clear + 8, salt);
		if (!cp) {
			perror("crypt");
			exit(1);
		}
		strcat(cipher, cp + 2);
	}
#endif	/* DOUBLESIZE */
#endif	/* SW_CRYPT */
	return cipher;
}
