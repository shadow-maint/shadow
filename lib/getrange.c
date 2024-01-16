/*
 * SPDX-FileCopyrightText: 2008       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <config.h>

#ident "$Id: $"

#include <ctype.h>
#include <stdlib.h>

#include "atoi/strtou_noneg.h"
#include "defines.h"
#include "prototypes.h"


/*
 * Parse a range and indicate if the range is valid.
 * Valid ranges are in the form:
 *     <long>          -> min=max=long         has_min  has_max
 *     -<long>         -> max=long            !has_min  has_max
 *     <long>-         -> min=long             has_min !has_max
 *     <long1>-<long2> -> min=long1 max=long2  has_min  has_max
 */
int
getrange(const char *range,
         unsigned long *min, bool *has_min,
         unsigned long *max, bool *has_max)
{
	char  *endptr;

	if (NULL == range)
		return -1;

	*has_min = false;
	*has_max = false;

	if ('-' == range[0]) {
		endptr = range + 1;
		goto parse_max;
	}

	errno = 0;
	*min = strtoul_noneg(range, &endptr, 10);
	if (endptr == range || 0 != errno)
		return -1;
	*has_min = true;

	switch (*endptr++) {
	case '\0':
		*has_max = true;
		*max = *min;
		return 0;  /* <long> */

	case '-':
		if ('\0' == *endptr)
			return 0;  /* <long>- */
parse_max:
		if (!isdigit(*endptr))
			return -1;

		errno = 0;
		*max = strtoul_noneg(endptr, &endptr, 10);
		if ('\0' != *endptr || 0 != errno)
			return -1;
		*has_max = true;

		return 0;  /* <long>-<long>, or -<long> */

	default:
		return -1;
	}
}
