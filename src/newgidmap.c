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

static void parse_gid_options(char **options)
{
	int i;

	if (NULL == options)
		return;

	for (i = 0; NULL != options[i]; i++) {
		char *option = options[i];

		if (strlen(option) < 1)
			continue;

		/* No options are currently valid for /etc/subgid, so error out. */

		fprintf(stderr, _("%s: option '%s' is not understood\n"),
			Prog,
			option);
		exit(EXIT_FAILURE);
	}
}

static bool verify_range(struct passwd *pw, struct map_range *range, bool *allow_setgroups)
{
	/* An empty range is invalid */
	if (range->count == 0)
		return false;

	/* Test /etc/subgid. If the mapping is valid then we allow setgroups. */
	if (have_sub_gids(pw->pw_name, range->lower, range->count)) {
		char **options = sub_gid_options(pw->pw_name, range->lower, range->count);

		*allow_setgroups = true;
		parse_gid_options(options);
		free_list(options);
		return true;
	}

	/* Allow a process to map its own gid. */
	if ((range->count == 1) && (pw->pw_gid == range->lower)) {
		/* noop -- if setgroups is enabled already we won't disable it. */
		return true;
	}

	return false;
}

static void verify_ranges(struct passwd *pw, int ranges,
	struct map_range *mappings, bool *allow_setgroups)
{
	struct map_range *mapping;
	int idx;

	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		if (!verify_range(pw, mapping, allow_setgroups)) {
			fprintf(stderr, _( "%s: gid range [%lu-%lu) -> [%lu-%lu) not allowed\n"),
				Prog,
				mapping->upper,
				mapping->upper + mapping->count,
				mapping->lower,
				mapping->lower + mapping->count);
			exit(EXIT_FAILURE);
		}
	}
}

static void usage(void)
{
	fprintf(stderr, _("usage: %s <pid> <gid> <lowergid> <count> [ <gid> <lowergid> <count> ] ... \n"), Prog);
	exit(EXIT_FAILURE);
}

void write_setgroups(int proc_dir_fd, bool allow_setgroups)
{
	int setgroups_fd;
	char *policy, policy_buffer[4096];

	/*
	 * Default is "deny", and any "allow" will out-rank a "deny". We don't
	 * forcefully write an "allow" here because the process we are writing
	 * mappings for may have already set themselves to "deny" (and "allow"
	 * is the default anyway). So allow_setgroups == true is a noop.
	 */
	policy = "deny\n";
	if (allow_setgroups)
		return;

	setgroups_fd = openat(proc_dir_fd, "setgroups", O_RDWR|O_CLOEXEC);
	if (setgroups_fd < 0) {
		/*
		 * If it's an ENOENT then we are on too old a kernel for the setgroups
		 * code to exist. Emit a warning and bail on this.
		 */
		if (ENOENT == errno) {
			fprintf(stderr, _("%s: kernel doesn't support setgroups restrictions\n"), Prog);
			goto out;
		}
		fprintf(stderr, _("%s: couldn't open process setgroups: %s\n"),
			Prog,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*
	 * Check whether the policy is already what we want. /proc/self/setgroups
	 * is write-once, so attempting to write after it's already written to will
	 * fail.
	 */
	if (read(setgroups_fd, policy_buffer, sizeof(policy_buffer)) < 0) {
		fprintf(stderr, _("%s: failed to read setgroups: %s\n"),
			Prog,
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!strncmp(policy_buffer, policy, strlen(policy)))
		goto out;

	/* Write the policy. */
	if (lseek(setgroups_fd, 0, SEEK_SET) < 0) {
		fprintf(stderr, _("%s: failed to seek setgroups: %s\n"),
			Prog,
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (dprintf(setgroups_fd, "%s", policy) < 0) {
		fprintf(stderr, _("%s: failed to setgroups %s policy: %s\n"),
			Prog,
			policy,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

out:
	close(setgroups_fd);
}

/*
 * newgidmap - Set the gid_map for the specified process
 */
int main(int argc, char **argv)
{
	char proc_dir_name[32];
	char *target_str;
	pid_t target, parent;
	int proc_dir_fd;
	int ranges;
	struct map_range *mappings;
	struct stat st;
	struct passwd *pw;
	int written;
	bool allow_setgroups = false;

	Prog = Basename (argv[0]);

	/*
	 * The valid syntax are
	 * newgidmap target_pid
	 */
	if (argc < 2)
		usage();

	/* Find the process that needs its user namespace
	 * gid mapping set.
	 */
	target_str = argv[1];
	if (!get_pid(target_str, &target))
		usage();

	/* max string length is 6 + 10 + 1 + 1 = 18, allocate 32 bytes */
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
		fprintf(stderr, _( "%s: Target %u is owned by a different user: uid:%lu pw_uid:%lu st_uid:%lu, gid:%lu pw_gid:%lu st_gid:%lu\n" ),
			Prog, target,
			(unsigned long int)getuid(), (unsigned long int)pw->pw_uid, (unsigned long int)st.st_uid,
			(unsigned long int)getgid(), (unsigned long int)pw->pw_gid, (unsigned long int)st.st_gid);
		return EXIT_FAILURE;
	}

	if (!sub_gid_open(O_RDONLY)) {
		return EXIT_FAILURE;
	}

	ranges = ((argc - 2) + 2) / 3;
	mappings = get_map_ranges(ranges, argc - 2, argv + 2);
	if (!mappings)
		usage();

	verify_ranges(pw, ranges, mappings, &allow_setgroups);

	write_setgroups(proc_dir_fd, allow_setgroups);
	write_mapping(proc_dir_fd, ranges, mappings, "gid_map");
	sub_gid_close();

	return EXIT_SUCCESS;
}
