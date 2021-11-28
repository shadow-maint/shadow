#include <stdio.h>
#include "subid.h"
#include "stdlib.h"
#include "prototypes.h"
#include "shadowlog.h"

const char *Prog;

void usage(void)
{
	fprintf(stderr, "Usage: [-g] %s subuid\n", Prog);
	fprintf(stderr, "    list uids who own the given subuid\n");
	fprintf(stderr, "    pass -g to query a subgid\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i, n;
	uid_t *uids;

	Prog = Basename (argv[0]);
	log_set_progname(Prog);
	log_set_logfd(stderr);
	if (argc < 2) {
		usage();
	}
	if (argc == 3 && strcmp(argv[1], "-g") == 0)
		n = subid_get_gid_owners(atoi(argv[2]), &uids);
	else if (argc == 2 && strcmp(argv[1], "-h") == 0)
		usage();
	else
		n = subid_get_uid_owners(atoi(argv[1]), &uids);
	if (n < 0) {
		fprintf(stderr, "No owners found\n");
		exit(1);
	}
	for (i = 0; i < n; i++) {
		printf("%d\n", uids[i]);
	}
	free(uids);
	return 0;
}
