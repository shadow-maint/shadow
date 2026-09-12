/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Check that the status a subid NSS module reports survives the public
 * libsubid API, and that a result which contradicts its status is dropped.
 * Run with nsswitch3.conf ("subid: zzz") bound over /etc/nsswitch.conf.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subid.h"

static int failures;

static void
check_ranges2(const char *owner, enum subid_status want_err, int want_count)
{
	struct subid_range *ranges;
	int count = 42;
	enum subid_status err = SUBID_STATUS_ERROR;

	ranges = subid_get_uid_ranges2(owner, &count, &err);
	if (err != want_err || count != want_count ||
	    (want_err == SUBID_STATUS_SUCCESS) != (ranges != NULL)) {
		printf("FAIL %s: err %d count %d ranges %s, want err %d count %d\n",
		       owner, err, count, ranges ? "set" : "NULL",
		       want_err, want_count);
		failures++;
	} else {
		printf("ok   %s: err %d count %d ranges %s\n",
		       owner, err, count, ranges ? "set" : "NULL");
	}
	subid_free(ranges);
}

static void
check_legacy(const char *owner, int want)
{
	struct subid_range *ranges = (struct subid_range *) 1;
	int n;

	n = subid_get_uid_ranges(owner, &ranges);
	if (n != want || (n <= 0 && ranges != NULL) || (n > 0 && ranges == NULL)) {
		printf("FAIL legacy %s: got %d ranges %s, want %d\n",
		       owner, n, ranges ? "set" : "NULL", want);
		failures++;
	} else {
		printf("ok   legacy %s: %d\n", owner, n);
	}
	subid_free(ranges);
}

int main(void)
{
	if (!subid_init("test_status", stderr))
		return 1;

	check_ranges2("user1", SUBID_STATUS_SUCCESS, 1);
	check_ranges2("user2", SUBID_STATUS_SUCCESS, 0);
	check_ranges2("unknown", SUBID_STATUS_UNKNOWN_USER, 0);
	check_ranges2("conn", SUBID_STATUS_ERROR_CONN, 0);
	check_ranges2("error", SUBID_STATUS_ERROR, 0);
	check_ranges2("errptr", SUBID_STATUS_ERROR, 0);
	check_ranges2("zeroptr", SUBID_STATUS_SUCCESS, 0);
	check_ranges2("negcount", SUBID_STATUS_ERROR, 0);

	check_legacy("user1", 1);
	check_legacy("user2", 0);
	check_legacy("unknown", -1);
	check_legacy("conn", -1);
	check_legacy("error", -1);
	check_legacy("errptr", -1);
	check_legacy("zeroptr", 0);
	check_legacy("negcount", -1);

	printf("status tests done, %d failure(s)\n", failures);
	return failures != 0;
}
