// This program is for testing purposes only.
// usage is "[program] owner [u|g] start count
// Exits 0 if owner has subid range starting start, of size count
// Exits 1 otherwise.


#include <config.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "atoi/strtoi.h"
#include "defines.h"
#include "prototypes.h"
#include "subordinateio.h"
#include "idmapping.h"
#include "shadowlog.h"


const char *Prog;


int main(int argc, char **argv)
{
	int            status;
	bool           check_uids;
	char           *owner;
	unsigned long  start, count;

	Prog = Basename (argv[0]);
	log_set_progname(Prog);
	log_set_logfd(stderr);

	if (argc != 5)
		exit(1);

	owner = argv[1];
	check_uids = argv[2][0] == 'u';
	start = strtonl(argv[3], NULL, 10, &status, unsigned long);
	if (status != 0)
		exit(1);
	count = strtonl(argv[4], NULL, 10, &status, unsigned long);
	if (status != 0)
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
