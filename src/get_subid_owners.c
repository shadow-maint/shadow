/* SPDX-License-Identifier: BSD-3-Clause */


#include <stdio.h>

#include "atoi/getnum.h"
#include "attr.h"
#include "io/fprintf.h"
#include "prototypes.h"
#include "stdlib.h"
#include "string/strcmp/streq.h"
#include "subid.h"


static const char Prog[] = "get_subid_owners";


NORETURN static void usage(void);


int main(int argc, char *argv[])
{
	int    i, n;
	uid_t  u;
	uid_t  *uids;

	if (!subid_init(Prog, stderr))
		eprinte("subid_init");
	if (argc < 2) {
		usage();
	}
	if (argc == 3 && streq(argv[1], "-g")) {
		get_uid(argv[2], &u);
		uids = subid_get_gid_owners(u, &n);
	} else if (argc == 2 && streq(argv[1], "-h")) {
		usage();
	} else {
		get_gid(argv[1], &u);
		uids = subid_get_uid_owners(u, &n);
	}
	if (uids == NULL) {
		eprinte("%s: cannot list the owners", Prog);
		exit(1);
	}
	for (i = 0; i < n; i++) {
		printf("%d\n", uids[i]);
	}
	subid_free(uids);
	return 0;
}


static void
usage(void)
{
	eprintf("Usage: [-g] %s subuid\n", Prog);
	eprintf("    list uids who own the given subuid\n");
	eprintf("    pass -g to query a subgid\n");
	exit(EXIT_FAILURE);
}
