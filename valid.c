/*
 * Copyright 1989, 1990, 1991, 1993, John F. Haugh II
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

#include <sys/types.h>
#include <stdio.h>
#ifdef	BSD
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#else
#include <string.h>
#include <memory.h>
#endif
#include "config.h"
#include "pwd.h"

#ifndef	lint
static	char	_sccsid[] = "@(#)valid.c	3.5	22:04:54	02 Jun 1993";
#endif

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

int
valid (password, entry)
char	*password;
struct	passwd	*entry;
{
	char	*encrypt;
	char	*salt;
	char	*pw_encrypt ();

	/*
	 * Start with blank or empty password entries.  Always encrypt
	 * a password if no such user exists.  Only if the ID exists and
	 * the password is really empty do you return quickly.  This
	 * routine is meant to waste CPU time.
	 */

	if (entry->pw_name && ! entry->pw_passwd[0]) {
		if (! password[0])
			return (1);	/* user entered nothing */
		else
			return (0);	/* user entered something! */
	}

	/*
	 * If there is no entry then we need a salt to use.
	 */

	if (entry->pw_name == (char *) 0 || entry->pw_passwd[0] == '\0')
		salt = "xx";
	else
		salt = entry->pw_passwd;

	/*
	 * Now, perform the encryption using the salt from before on
	 * the users input.  Since we always encrypt the string, it
	 * should be very difficult to determine if the user exists by
	 * looking at execution time.
	 */

	encrypt = pw_encrypt (password, salt);

	/*
	 * One last time we must deal with there being no password file
	 * entry for the user.  We use the pw_passwd == NULL idiom to
	 * cause non-existent users to not be validated.
	 */

	if (entry->pw_name && strcmp (encrypt, entry->pw_passwd) == 0)
		return (1);
	else
		return (0);
}
