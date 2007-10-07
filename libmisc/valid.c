/*
 * Copyright 1989 - 1993, Julianne Frances Haugh
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

#ident "$Id: valid.c,v 1.6 2005/08/31 17:24:58 kloczek Exp $"

#include <sys/types.h>
#include <stdio.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>
/*
 * valid - compare encrypted passwords
 *
 *	Valid() compares the DES encrypted password from the password file
 *	against the password which the user has entered after it has been
 *	encrypted using the same salt as the original.  Entries which do
 *	not have a password file entry have a NULL pw_name field and this
 *	is used to indicate that a dummy salt must be used to encrypt the
 *	password anyway.
 */
int valid (const char *password, const struct passwd *ent)
{
	const char *encrypted;
	const char *salt;

	/*
	 * Start with blank or empty password entries.  Always encrypt
	 * a password if no such user exists.  Only if the ID exists and
	 * the password is really empty do you return quickly.  This
	 * routine is meant to waste CPU time.
	 */

	if (ent->pw_name && !ent->pw_passwd[0]) {
		if (!password[0])
			return (1);	/* user entered nothing */
		else
			return (0);	/* user entered something! */
	}

	/*
	 * If there is no entry then we need a salt to use.
	 */

	if (ent->pw_name == (char *) 0 || ent->pw_passwd[0] == '\0') {
		salt = "xx";
	} else {
		salt = ent->pw_passwd;
	}

	/*
	 * Now, perform the encryption using the salt from before on
	 * the users input.  Since we always encrypt the string, it
	 * should be very difficult to determine if the user exists by
	 * looking at execution time.
	 */

	encrypted = pw_encrypt (password, salt);

	/*
	 * One last time we must deal with there being no password file
	 * entry for the user.  We use the pw_name == NULL idiom to
	 * cause non-existent users to not be validated.
	 */

	if (ent->pw_name && strcmp (encrypted, ent->pw_passwd) == 0)
		return (1);
	else
		return (0);
}
