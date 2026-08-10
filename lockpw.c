/*
 * Copyright 1992, John F. Haugh II
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

#ifndef	lint
static	char	sccsid[] = "@(#)lockpw.c	3.1	08:12:51	12 Nov 1992";
#endif

/*
 * lckpwdf - lock the password files
 */

int
lckpwdf ()
{
	int	i;

	/*
	 * We have 15 seconds to lock the whole mess
	 */

	for (i = 0;i < 15;i++)
		if (pw_lock ())
			break;
		else
			sleep (1);

	/*
	 * Did we run out of time?
	 */

	if (i == 15)
		return -1;

	/*
	 * Nope, use any remaining time to lock the shadow password
	 * file.
	 */

	for (;i < 15;i++)
		if (spw_lock ())
			break;
		else
			sleep (1);

	/*
	 * Out of time yet?
	 */

	if (i == 15) {
		pw_unlock ();
		return -1;
	}

	/*
	 * Nope - and both files are now locked.
	 */

	return 0;
}

/*
 * ulckpwdf - unlock the password files
 */

int
ulckpwdf ()
{

	/*
	 * Unlock both files.
	 */

	return (pw_unlock () && spw_unlock ()) ? 0:-1;
}
