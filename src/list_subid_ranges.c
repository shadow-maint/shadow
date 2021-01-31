#include <stdio.h>
#include "api.h"
#include "stdlib.h"
#include "prototypes.h"

const char *Prog;

void usage(void)
{
	fprintf(stderr, "Usage: %s [-g] user\n", Prog);
	fprintf(stderr, "    list subuid ranges for user\n");
	fprintf(stderr, "    pass -g to list subgid ranges\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, count=0;
	struct subordinate_range **ranges;

	Prog = Basename (argv[0]);
	if (argc < 2) {
		usage();
	}
	if (argc == 3 && strcmp(argv[1], "-g") == 0)
		count = get_subgid_ranges(argv[2], &ranges);
	else if (argc == 2 && strcmp(argv[1], "-h") == 0)
		usage();
	else
		count = get_subuid_ranges(argv[1], &ranges);
	if (!ranges) {
		fprintf(stderr, "Error fetching ranges\n");
		exit(1);
	}
	for (i = 0; i < count; i++) {
		printf("%d: %s %lu %lu\n", i, ranges[i]->owner,
			ranges[i]->start, ranges[i]->count);
	}
	subid_free_ranges(ranges, count);
	return 0;
}
