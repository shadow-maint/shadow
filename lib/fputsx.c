/*
 * SPDX-FileCopyrightText: 1990 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1999, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2005       , Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#include <stdio.h>
#include "defines.h"
#include "prototypes.h"

#ident "$Id$"


/*
 * Similar to getline(3), but handle escaped newlines.
 * Returns the length of the string, or -1 on error.
 */
ssize_t
getlinex(char **restrict lineptr, size_t *restrict size,
         FILE *restrict stream)
{
	char    *p;
	size_t  len;

	if (*lineptr == NULL || *size < BUFSIZ) {
		*size = BUFSIZ;
	} else if (*size > INT_MAX) {
		*size = INT_MAX;
	}

	len = 0;
	for (size_t sz = *size; sz < INT_MAX; sz <<= 1) {
		*size = sz;
		*lineptr = reallocf(*lineptr, sz);
		if (*lineptr == NULL)
			return -1;

		p = fgetsnulx(*lineptr + len, sz, stream);
		if (p == NULL)
			return -1;

		len = p - *lineptr;
		if (p[-1] == '\n' && p[-2] != '\\')
			return len;
		if (p[-1] == '\n' && p[-2] == '\\')
			len -= 2;
	}

	return -1;
}


/* Like fgetsx(), but returns a pointer to the terminating NUL byte.  */
char *
fgetsnulx(char *buf, int size, FILE *stream)
{
	if (fgetsx(buf, size, stream) == NULL)
		return NULL;
	return buf + strlen(buf);
}


/*@null@*/char *fgetsx (/*@returned@*/ /*@out@*/char *buf, int cnt, FILE * f)
{
	char *cp = buf;
	char *ep;

	while (cnt > 0) {
		if (fgets (cp, cnt, f) != cp) {
			if (cp == buf) {
				return 0;
			} else {
				break;
			}
		}
		ep = strrchr (cp, '\\');
		if ((NULL != ep) && (*(ep + 1) == '\n')) {
			cnt -= ep - cp;
			if (cnt > 0) {
				cp = ep;
				*cp = '\0';
			}
		} else {
			break;
		}
	}
	return buf;
}

int fputsx (const char *s, FILE * stream)
{
	int i;

	for (i = 0; '\0' != *s; i++, s++) {
		if (putc (*s, stream) == EOF) {
			return EOF;
		}

#if 0				/* The standard getgr*() can't handle that.  --marekm */
		if (i > (BUFSIZ / 2)) {
			if (putc ('\\', stream) == EOF ||
			    putc ('\n', stream) == EOF)
				return EOF;

			i = 0;
		}
#endif
	}
	return 0;
}

