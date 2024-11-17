/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prototypes.h"
#include "shadowlog.h"
#include "string/strcmp/streq.h"
#include "subid.h"

static const char Prog[] = "getsubids";

static void usage(void)
{
	fprintf(stderr, "Usage: %s [-g] user\n", Prog);
	fprintf(stderr, "    list subuid ranges for user\n");
	fprintf(stderr, "    pass -g to list subgid ranges\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, count=0;
	struct subid_range *ranges;
	const char *owner;

	log_set_progname(Prog);
	log_set_logfd(stderr);
	if (argc < 2)
		usage();
	owner = argv[1];
	if (argc == 3 && streq(argv[1], "-g")) {
		owner = argv[2];
		count = subid_get_gid_ranges(owner, &ranges);
	} else if (argc == 2 && streq(argv[1], "-h")) {
		usage();
	} else {
		count = subid_get_uid_ranges(owner, &ranges);
	}
	if (!ranges) {
		fprintf(stderr, "Error fetching ranges\n");
		exit(1);
	}
	for (i = 0; i < count; i++) {
		printf("%d: %s %lu %lu\n", i, owner,
			ranges[i].start, ranges[i].count);
	}
	subid_free(ranges);
	return 0;
}
