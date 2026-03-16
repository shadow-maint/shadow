/*
 * SPDX-FileCopyrightText: 2012 Eric Biederman
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#ifdef ENABLE_SUBIDS

#include <stdio.h>
#include <errno.h>

#include "prototypes.h"
#include "subordinateio.h"
#include "getdef.h"
#include "shadowlog.h"

#undef NDEBUG
#include <assert.h>


/*
 * find_new_sub_uids_linear - Find an unused subordinate UID range via
 * linear search.
 *
 * Loads SUB_UID_COUNT from login.defs and writes the allocated count back
 * to *range_count on success.
 *
 * Return 0 on success, -1 if no unused UIDs are available.
 */
static int
find_new_sub_uids_linear(id_t *range_start, unsigned long *range_count)
{
	unsigned long min, max;
	unsigned long count;
	id_t start;

	min = getdef_ulong ("SUB_UID_MIN", 100000UL);
	max = getdef_ulong ("SUB_UID_MAX", 600100000UL);
	count = getdef_ulong ("SUB_UID_COUNT", 65536);

	start = sub_uid_find_free_range(min, max, count);
	if (start == -1)
		return -1;

	*range_start = start;
	*range_count = count;
	return 0;
}

/*
 * find_new_sub_uids - Find a new unused range of subordinate UIDs.
 *
 * Return 0 on success, -1 if no unused UIDs are available.
 */
int
find_new_sub_uids(id_t *range_start, unsigned long *range_count)
{
	return find_new_sub_uids_linear(range_start, range_count);
}

#else				/* !ENABLE_SUBIDS */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !ENABLE_SUBIDS */
