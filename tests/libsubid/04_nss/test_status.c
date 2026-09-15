/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include "subid.h"

static int  failures;

// check: 'want_count' ranges, or NULL with errno 'want_errno'
static void
check(enum subid_type type, const char *owner, int want_errno, int want_count)
{
	int                 count;
	bool                ok;
	struct subid_range  *ranges;

	if (type == ID_TYPE_UID)
		ranges = subid_get_uid_ranges2(owner, &count);
	else
		ranges = subid_get_gid_ranges2(owner, &count);
	if (want_errno == 0)
		ok = ranges != NULL;
	else
		ok = ranges == NULL && errno == want_errno;
	if (!ok || count != want_count) {
		printf("FAIL %s %s: %s, errno %d, count %d; want %d, %d\n",
		       type == ID_TYPE_UID ? "uid" : "gid", owner,
		       ranges ? "array" : "NULL", errno, count, want_errno,
		       want_count);
		failures++;
	}
	subid_free(ranges);
}

int
main(void)
{
	if (!subid_init("test_status", stderr))
		return 1;

	check(ID_TYPE_UID, "user1", 0, 1);
	check(ID_TYPE_UID, "user2", 0, 0);
	check(ID_TYPE_UID, "emptyarr", 0, 0);
	check(ID_TYPE_UID, "unknown", ENOENT, 0);
	check(ID_TYPE_UID, "conn", EAGAIN, 0);
	check(ID_TYPE_UID, "error", EIO, 0);
	check(ID_TYPE_GID, "group1", 0, 1);
	check(ID_TYPE_GID, "user1", 0, 0);
	check(ID_TYPE_GID, "unknown", ENOENT, 0);

	return failures != 0;
}
