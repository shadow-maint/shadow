/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attr.h"
#include "io/fprintf/eprintf.h"
#include "prototypes.h"
#include "string/strcmp/streq.h"
#include "string/strerrno.h"
#include "subid.h"

static const char Prog[] = "getsubids";


NORETURN static void usage(void);


int main(int argc, char *argv[])
{
	int i, count=0;
	struct subid_range *ranges;
	const char *owner;

	if (!subid_init(Prog, stderr))
		fprintf(stderr, "subid_init: %s\n", strerrno());
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
		eprintf("Error fetching ranges\n");
		exit(1);
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
