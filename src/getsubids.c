/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attr.h"
#include "io/fprintf.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "subid.h"

/*
 * Exit statuses.  An owner without ranges is a success with no output.
 */
#define E_LOOKUP_FAILED   1	/* the lookup failed */
#define E_UNKNOWN_OWNER   2	/* the backend does not know the owner */
#define E_BACKEND_DOWN    3	/* the backend could not be reached */

static const char Prog[] = "getsubids";


NORETURN static void usage(void);


int main(int argc, char *argv[])
{
	int i, count = 0;
	struct subid_range *ranges = NULL;
	const char *owner;
	enum subid_status err = SUBID_STATUS_ERROR;

	if (!subid_init(Prog, stderr))
		eprinte("subid_init");
	if (argc < 2)
		usage();
	owner = argv[1];
	if (argc == 3 && streq(argv[1], "-g")) {
		owner = argv[2];
		ranges = subid_get_gid_ranges2(owner, &count, &err);
	} else if (argc == 2 && streq(argv[1], "-h")) {
		usage();
	} else {
		ranges = subid_get_uid_ranges2(owner, &count, &err);
	}
	if (ranges == NULL) {
		switch (err) {
		case SUBID_STATUS_UNKNOWN_USER:
			eprintf("%s: unknown owner: %s\n", Prog, owner);
			exit(E_UNKNOWN_OWNER);
		case SUBID_STATUS_ERROR_CONN:
			eprintf("%s: could not reach the subordinate ID backend\n", Prog);
			exit(E_BACKEND_DOWN);
		default:
			eprintf("Error fetching ranges\n");
			exit(E_LOOKUP_FAILED);
		}
	}
	for (i = 0; i < count; i++) {
		printf("%d: %s %lu %lu\n", i, owner,
			ranges[i].start, ranges[i].count);
	}
	subid_free(ranges);
	return 0;
}


static void
usage(void)
{
	eprintf("Usage: %s [-g] user\n", Prog);
	eprintf("    list subuid ranges for user\n");
	eprintf("    pass -g to list subgid ranges\n");
	exit(EXIT_FAILURE);
}
