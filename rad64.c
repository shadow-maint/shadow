/*
 * Copyright 1989, 1990, 1992, John F. Haugh II
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 */

#include "config.h"

#ifndef	lint
static	char	sccsid[] = "@(#)rad64.c	3.3	20:38:01	07 Mar 1992";
#endif

/*
 * c64i - convert a radix 64 character to an integer
 */

int	c64i (c)
char	c;
{
	if (c == '.')
		return (0);

	if (c == '/')
		return (1);

	if (c >= '0' && c <= '9')
		return (c - '0' + 2);

	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 12);

	if (c >= 'a' && c <= 'z')
		return (c - 'a' + 38);
	else
		return (-1);
}

/*
 * i64c - convert an integer to a radix 64 character
 */

int	i64c (i)
int	i;
{
	if (i < 0)
		return ('.');
	else if (i > 63)
		return ('z');

	if (i == 0)
		return ('.');

	if (i == 1)
		return ('/');

	if (i >= 2 && i <= 11)
		return ('0' - 2 + i);

	if (i >= 12 && i <= 37)
		return ('A' - 12 + i);

	if (i >= 38 && i <= 63)
		return ('a' - 38 + i);

	return ('\0');
}

#ifdef NEED_AL64

/*
 * l64a - convert a long to a string of radix 64 characters
 */

char	*l64a (l)
long	l;
{
	static	char	buf[8];
	int	i = 0;

	if (i < 0L)
		return ((char *) 0);

	do {
		buf[i++] = i64c ((int) (l % 64));
		buf[i] = '\0';
	} while (l /= 64L, l > 0 && i < 6);

	return (buf);
}

/*
 * a64l - convert a radix 64 string to a long integer
 */

long	a64l (s)
char	*s;
{
	int	i;
	long	value;
	long	shift = 0;

	for (i = 0, value = 0L;i < 6 && *s;s++) {
		value += (c64i (*s) << shift);
		shift += 6;
	}
	return (value);
}

#endif /* NEED_A64L */
