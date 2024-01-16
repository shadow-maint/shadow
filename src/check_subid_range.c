// This program is for testing purposes only.
// usage is "[program] owner [u|g] start count
// Exits 0 if owner has subid range starting start, of size count
// Exits 1 otherwise.

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "atoi/getnum.h"
#include "atoi/str2i.h"
#include "defines.h"
#include "prototypes.h"
#include "subordinateio.h"
#include "idmapping.h"
#include "shadowlog.h"

const char *Prog;

int main(int argc, char **argv)
{
	bool           check_uids;
	char           *owner;
	uid_t          start;
	unsigned long  count;

	Prog = Basename (argv[0]);
	log_set_progname(Prog);
	log_set_logfd(stderr);

	if (argc != 5)
		exit(1);

	owner = argv[1];
	check_uids = argv[2][0] == 'u';
	errno = 0;
	if (get_uid(argv[3], &start) == -1)
		exit(1);
	if (str2ul(argv[4], &count) == -1)
		exit(1);
	if (check_uids) {
		if (have_sub_uids(owner, start, count))
			exit(0);
		exit(1);
	}
	if (have_sub_gids(owner, start, count))
		exit(0);
	exit(1);
}
