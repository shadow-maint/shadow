/*
 * Copyright 1989, 1990, 1991, 1992, 1993, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

/*
 * This version of obscure.c contains modifications to support "cracklib"
 * by Alec Muffet (alec.muffett@uk.sun.com).  You must obtain the Cracklib
 * library source code for this function to operate.
 */

#include <ctype.h>
#ifndef	BSD
#include <string.h>
#include <memory.h>
#else
#include <strings.h>
#define	strchr	index
#define	strrchr	rindex
#endif
#include "config.h"

#ifndef	lint
static	char	sccsid[] = "@(#)obscure.c	3.7	18:10:59	03 Jul 1993";
#endif

extern	int	getdef_bool();
extern	int	getdef_num();

#ifdef	NEED_STRSTR
/*
 * strstr - find substring in string
 */

char *
strstr (string, pattern)
char	*string;
char	*pattern;
{
	char	*cp;
	int	len;

	len = strlen (pattern);

	for (cp = string;cp = strchr (cp, *pattern);) {
		if (strncmp (cp, pattern, len) == 0)
			return cp;

		cp++;
	}
	return 0;
}
#endif

/*
 * Obscure - see if password is obscure enough.
 *
 *	The programmer is encouraged to add as much complexity to this
 *	routine as desired.  Included are some of my favorite ways to
 *	check passwords.
 */

/*ARGSUSED*/
int	obscure (old, new)
char	*old;
char	*new;
{
	int	i;
	char	oldmono[32];
	char	newmono[32];
	char	wrapped[64];
#ifdef	CRACKLIB_DICTPATH
	char	*msg;
	char	*FascistCheck();
#endif

	if (old[0] == '\0')
		return (1);

	if ( strlen(new) < getdef_num("PASS_MIN_LEN", 0) ) {
		printf ("Too short.  ");
		return (0);
	}

	/*
	 * Remaining checks are optional.
	 */
	if ( !getdef_bool("OBSCURE_CHECKS_ENAB") )
		return (1);

	for (i = 0;new[i];i++)
		newmono[i] = tolower (new[i]);

	for (i = 0;old[i];i++)
		oldmono[i] = tolower (old[i]); 

	if (strcmp (new, old) == 0) {	/* the same */
		printf ("No Change.  ");
		return (0);
	}
	if (palindrome (newmono, oldmono))	/* a palindrome */
		return (0);

	if (strcmp (newmono, oldmono) == 0) {	/* case shifted */
		printf ("Case changes only.  ");
		return (0);
	}
	if (similiar (newmono, oldmono))	/* jumbled version */
		return (0);

	if (simple (old, new))			/* keyspace size */
		return (0);

	strcpy (wrapped, oldmono);
	strcat (wrapped, oldmono);
	if (strstr (wrapped, newmono)) {
		printf ("Rotated.  ");
		return (0);
	}
#ifdef CRACKLIB_DICTPATH	

	/*
	 * Invoke Alec Muffett's cracklib routines.
	 */

	if (msg = FascistCheck (new, CRACKLIB_DICTPATH)) {
		printf("Bad Password: %s.  ", msg);
		return (0);
	}
#endif /* CRACKLIB_DICTPATH */

	return (1);
}

/*
 * can't be a palindrome - like `R A D A R' or `M A D A M'
 */

/*ARGSUSED*/
int	palindrome (old, new)
char	*old;
char	*new;
{
	int	i, j;

	i = strlen (new);

	for (j = 0;j < i;j++)
		if (new[i - j - 1] != new[j])
			return (0);

	printf ("A palindrome.  ");
	return (1);
}

/*
 * more than half of the characters are different ones.
 */

/*ARGSUSED*/
int	similiar (old, new)
char	*old;
char	*new;
{
	int	i, j;
	char	*strchr ();

	for (i = j = 0;new[i] && old[i];i++)
		if (strchr (new, tolower (old[i])))
			j++;

	if (i >= j * 2)
		return (0);

	printf ("Too similiar.  ");
	return (1);
}

/*
 * a nice mix of characters.
 */

/*ARGSUSED*/
int	simple (old, new)
char	*old;
char	*new;
{
	int	digits = 0;
	int	uppers = 0;
	int	lowers = 0;
	int	others = 0;
	int	size;
	int	i;

	for (i = 0;new[i];i++) {
		if (isdigit (new[i]))
			digits++;
		else if (isupper (new[i]))
			uppers++;
		else if (islower (new[i]))
			lowers++;
		else
			others++;
	}

	/*
	 * The scam is this - a password of only one character type
	 * must be 8 letters long.  Two types, 7, and so on.
	 */

	size = 9;
	if (digits) size--;
	if (uppers) size--;
	if (lowers) size--;
	if (others) size--;

	if (size <= i)
		return 0;

	printf ("Too Simple.  Use a longer password, or a mix of upper\n");
	printf ("and lower case letters and numerics.  ");
	return 1;
}
