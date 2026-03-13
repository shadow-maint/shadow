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


/*
 * find_new_sub_uids_deterministic - Assign a subordinate UID range by UID.
 *
 * Calculates a deterministic subordinate UID range for a given UID based
 * on its offset from UID_MIN.  Loads SUB_UID_COUNT from login.defs and
 * writes it back to *range_count on success.
 *
 * BASE FORMULA:
 *   uid_offset     = uid - UID_MIN
 *   logical_offset = uid_offset * SUB_UID_COUNT
 *   start_id       = SUB_UID_MIN + logical_offset
 *   end_id         = start_id + SUB_UID_COUNT - 1
 *
 * DETERMINISTIC-SAFE MODE (default):
 *   All arithmetic overflow is a hard error.  The assigned range must fit
 *   entirely within [SUB_UID_MIN, SUB_UID_MAX].  Allocation is monotonic
 *   and guaranteed non-overlapping.
 *
 * UNSAFE_SUB_UID_DETERMINISTIC_WRAP MODE:
 *   Activated with UNSAFE_SUB_UID_DETERMINISTIC_WRAP yes
 *
 *   WARNING: SECURITY RISK!
 *   WARNING: MAY CAUSE RANGE OVERLAPS!
 *   WARNING: MAY CAUSE CONTAINER ESCAPES!
 *
 *   The subordinate UID space is treated as a ring.  Arithmetic overflow
 *   is normalised via modulo over [SUB_UID_MIN, SUB_UID_MAX].
 *   This means ranges MAY overlap for large UID populations!
 *   Intended only for development, testing, or constrained lab environments.
 *
 * Return 0 on success, -1 if no UIDs are available.
 */
static int
find_new_sub_uids_deterministic(uid_t uid,
				id_t *range_start,
				unsigned long *range_count)
{
	bool           allow_wrap;
	unsigned long  count;
	unsigned long  slot;
	unsigned long  slots;
	unsigned long  space;
	unsigned long  uid_min;
	unsigned long  uid_offset;
	unsigned long  sub_uid_max;
	unsigned long  sub_uid_min;

	uid_min = getdef_ulong ("UID_MIN", 1000UL);
	sub_uid_min = getdef_ulong ("SUB_UID_MIN", 65536UL);
	sub_uid_max = getdef_ulong ("SUB_UID_MAX", 4294967295UL);
	count = getdef_ulong ("SUB_UID_COUNT", 65536UL);
	allow_wrap = getdef_bool ("UNSAFE_SUB_UID_DETERMINISTIC_WRAP");

	if (uid < uid_min) {
		fprintf(log_get_logfd(),
		         _("%s: UID %ju is less than UID_MIN %lu,"
		           " cannot calculate deterministic subordinate UIDs\n"),
		         log_get_progname(),
		         (uintmax_t)uid, uid_min);
		return -1;
	}

	if (sub_uid_min > sub_uid_max || count == 0) {
		fprintf(log_get_logfd(),
		         _("%s: Invalid configuration: SUB_UID_MIN (%lu),"
		           " SUB_UID_MAX (%lu), SUB_UID_COUNT (%lu)\n"),
		         log_get_progname(),
		         sub_uid_min, sub_uid_max, count);
		return -1;
	}

	if (__builtin_add_overflow(sub_uid_max - sub_uid_min, 1UL, &space)) {
		fprintf(log_get_logfd(),
			_("%s: SUB_UID range [%lu, %lu] is too large"
			  " to represent\n"),
			log_get_progname(),
			sub_uid_min, sub_uid_max);
		return -1;
	}

	if (count > space) {
		fprintf(log_get_logfd(),
		         _("%s: Not enough space for any subordinate UIDs"
		           " (SUB_UID_MIN=%lu, SUB_UID_MAX=%lu,"
		           " SUB_UID_COUNT=%lu)\n"),
		         log_get_progname(),
		         sub_uid_min, sub_uid_max, count);
		return -1;
	}

	uid_offset = uid - uid_min;
	slots = space / count;
	slot = uid_offset;

	if (uid_offset >= slots) {
		if (allow_wrap) {
			slot = uid_offset % slots;
		} else {
			fprintf(log_get_logfd(),
				_("%s: Deterministic subordinate UID range"
				  " for UID %ju exceeds SUB_UID_MAX (%lu)\n"),
				log_get_progname(),
				(uintmax_t)uid, sub_uid_max);
			return -1;
		}
	}

	*range_start = sub_uid_min + slot * count;
	*range_count = count;
	return 0;
}

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
find_new_sub_uids(uid_t uid, id_t *range_start, unsigned long *range_count)
{
	if (getdef_bool("SUB_UID_DETERMINISTIC"))
		return find_new_sub_uids_deterministic(uid, range_start, range_count);

	return find_new_sub_uids_linear(range_start, range_count);
}

#else				/* !ENABLE_SUBIDS */
extern int ISO_C_forbids_an_empty_translation_unit;
#endif				/* !ENABLE_SUBIDS */
