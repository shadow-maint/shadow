/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "subid.h"

static int  failures;

// check_ranges: 'want' ranges, and an array iff the lookup succeeded
static void
check_ranges(const char *owner, int want, unsigned long start,
    unsigned long count)
{
	int                 n;
	struct subid_range  *r;

	n = subid_get_uid_ranges(owner, &r);
	if (n != want || (n == -1) != (r == NULL)) {
		printf("FAIL ranges %s: got %d, %s; want %d\n",
		       owner, n, r ? "array" : "NULL", want);
		failures++;
	} else if (n > 0 && (r[0].start != start || r[0].count != count)) {
		printf("FAIL ranges %s: got %lu+%lu, want %lu+%lu\n",
		       owner, r[0].start, r[0].count, start, count);
		failures++;
	}
	subid_free(r);
}

// check_owners: 'want' owners, and an array iff the lookup succeeded
static void
check_owners(uid_t id, int want)
{
	int    n;
	uid_t  *uids;

	n = subid_get_uid_owners(id, &uids);
	if (n != want || (n == -1) != (uids == NULL)) {
		printf("FAIL owners %ju: got %d, %s; want %d\n",
		       (uintmax_t) id, n, uids ? "array" : "NULL", want);
		failures++;
	}
	subid_free(uids);
}

int
main(void)
{
	if (!subid_init("test_arrays", stderr))
		return 1;

	check_ranges("user1", 1, 100000, 65536);
	check_ranges("user2", 0, 0, 0);
	check_ranges("emptyarr", 0, 0, 0);
	check_ranges("unknown", -1, 0, 0);
	check_ranges("conn", -1, 0, 0);
	check_ranges("error", -1, 0, 0);

	check_owners(100000, 1);
	check_owners(5, 0);

	return failures != 0;
}
