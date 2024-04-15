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
		if (!isdigit(range[1]))
			return -1;

		errno = 0;
		*max = strtoul_noneg(&range[1], &endptr, 10);
		if (('\0' != *endptr) || (0 != errno))
			return -1;
		*has_max = true;

		return 0;  /* -<long> */
	}

	errno = 0;
	*min = strtoul_noneg(range, &endptr, 10);
	if (endptr == range || 0 != errno)
		return -1;
	*has_min = true;

	switch (*endptr) {
	case '\0':
		/* <long> */
		*has_max = true;
		*max = *min;
		break;
	case '-':
		endptr++;
		if ('\0' == *endptr)
			return 0;  /* <long>- */
		if (!isdigit(*endptr))
			return -1;

		errno = 0;
		*max = strtoul_noneg(endptr, &endptr, 10);
		if ('\0' != *endptr || 0 != errno)
			return -1;
		*has_max = true;

		/* <long>-<long> */
		break;
	default:
		return -1;
	}

	return 0;
}
