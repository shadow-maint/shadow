/*
 * Copyright (c) 2013 Eric Biederman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "defines.h"
#include "prototypes.h"
#include "subordinateio.h"
#include "idmapping.h"

/*
 * Global variables
 */
const char *Prog;

static bool verify_range(struct passwd *pw, struct map_range *range)
{
	/* An empty range is invalid */
	if (range->count == 0)
		return false;

	/* Test /etc/subuid */
	if (have_sub_uids(pw->pw_name, range->lower, range->count))
		return true;

	/* Allow a process to map it's own uid */
	if ((range->count == 1) && (pw->pw_uid == range->lower))
		return true;

	return false;
}

static void verify_ranges(struct passwd *pw, int ranges,
	struct map_range *mappings)
{
	struct map_range *mapping;
	int idx;

	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		if (!verify_range(pw, mapping)) {
			fprintf(stderr, _( "%s: uid range [%lu-%lu) -> [%lu-%lu) not allowed\n"),
				Prog,
				mapping->upper,
				mapping->upper + mapping->count,
				mapping->lower,
				mapping->lower + mapping->count);
			exit(EXIT_FAILURE);
		}
	}
}

void usage(void)
{
	fprintf(stderr, _("usage: %s <pid> <uid> <loweruid> <count> [ <uid> <loweruid> <count> ] ... \n"), Prog);
	exit(EXIT_FAILURE);
}

/*
 * newuidmap - Set the uid_map for the specified process
 */
int main(int argc, char **argv)
{
	char proc_dir_name[PATH_MAX];
	char *target_str;
	pid_t target, parent;
	int proc_dir_fd;
	int ranges;
	struct map_range *mappings;
	struct stat st;
	struct passwd *pw;
	int written;

	Prog = Basename (argv[0]);

	/*
	 * The valid syntax are
	 * newuidmap target_pid
	 */
	if (argc < 2)
		usage();

	/* Find the process that needs it's user namespace
	 * uid mapping set.
	 */
	target_str = argv[1];
	if (!get_pid(target_str, &target))
		usage();

	written = snprintf(proc_dir_name, sizeof(proc_dir_name), "/proc/%u/",
		target);
	if ((written <= 0) || (written >= sizeof(proc_dir_name))) {
		fprintf(stderr, "%s: snprintf of proc path failed: %s\n",
			Prog, strerror(errno));
	}

	proc_dir_fd = open(proc_dir_name, O_DIRECTORY);
	if (proc_dir_fd < 0) {
		fprintf(stderr, _("%s: Could not open proc directory for target %u\n"),
			Prog, target);
		return EXIT_FAILURE;
	}

	/* Who am i? */
	pw = get_my_pwent ();
	if (NULL == pw) {
		fprintf (stderr,
			_("%s: Cannot determine your user name.\n"),
			Prog);
		SYSLOG ((LOG_WARN, "Cannot determine the user name of the caller (UID %lu)",
				(unsigned long) getuid ()));
		return EXIT_FAILURE;
	}
	
	/* Get the effective uid and effective gid of the target process */
	if (fstat(proc_dir_fd, &st) < 0) {
		fprintf(stderr, _("%s: Could not stat directory for target %u\n"),
			Prog, target);
		return EXIT_FAILURE;
	}

	/* Verify real user and real group matches the password entry
	 * and the effective user and group of the program whose
	 * mappings we have been asked to set.
	 */
	if ((getuid() != pw->pw_uid) ||
	    (getgid() != pw->pw_gid) ||
	    (pw->pw_uid != st.st_uid) ||
	    (pw->pw_gid != st.st_gid)) {
		fprintf(stderr, _( "%s: Target %u is owned by a different user\n" ),
			Prog, target);
		return EXIT_FAILURE;
	}

	if (!sub_uid_open(O_RDONLY)) {
		return EXIT_FAILURE;
	}

	ranges = ((argc - 2) + 2) / 3;
	mappings = get_map_ranges(ranges, argc - 2, argv + 2);
	if (!mappings)
		usage();

	verify_ranges(pw, ranges, mappings);

	write_mapping(proc_dir_fd, ranges, mappings, "uid_map");
	sub_uid_close();

	return EXIT_SUCCESS;
}
