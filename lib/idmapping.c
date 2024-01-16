/*
 * SPDX-FileCopyrightText: 2013 Eric Biederman
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "alloc.h"
#include "atoi/a2i.h"
#include "prototypes.h"
#include "string/stpeprintf.h"
#include "idmapping.h"
#if HAVE_SYS_CAPABILITY_H
#include <sys/prctl.h>
#include <sys/capability.h>
#endif
#include "shadowlog.h"
#include "sizeof.h"


struct map_range *
get_map_ranges(int ranges, int argc, char **argv)
{
	struct map_range  *mappings, *m;

	if (ranges < 0 || argc < 0) {
		fprintf(log_get_logfd(), "%s: error calculating number of arguments\n", log_get_progname());
		return NULL;
	}

	if (ranges * 3 != argc) {
		fprintf(log_get_logfd(), "%s: ranges: %u is wrong for argc: %d\n", log_get_progname(), ranges, argc);
		return NULL;
	}

	mappings = CALLOC(ranges, struct map_range);
	if (!mappings) {
		fprintf(log_get_logfd(), _( "%s: Memory allocation failure\n"),
			log_get_progname());
		return NULL;
	}

	/* Gather up the ranges from the command line */
	m = mappings;
	for (int i = 0; i < ranges * 3; i+=3, m++) {
		if (a2ul(argv[i + 0], &m->upper, NULL, 0, 0, UINT_MAX) == -1) {
			if (errno == ERANGE)
				fprintf(log_get_logfd(), _( "%s: subuid overflow detected.\n"), log_get_progname());
			free(mappings);
			return NULL;
		}
		if (a2ul(argv[i + 1], &m->lower, NULL, 0, 0, UINT_MAX) == -1) {
			if (errno == ERANGE)
				fprintf(log_get_logfd(), _( "%s: subuid overflow detected.\n"), log_get_progname());
			free(mappings);
			return NULL;
		}
		if (a2ul(argv[i + 2], &m->count, NULL, 0, 0,
		         MIN(MIN(UINT_MAX, ULONG_MAX - 1) - m->lower,
		             MIN(UINT_MAX, ULONG_MAX - 1) - m->upper))
		    == -1)
		{
			if (errno == ERANGE)
				fprintf(log_get_logfd(), _( "%s: subuid overflow detected.\n"), log_get_progname());
			free(mappings);
			return NULL;
		}
	}
	return mappings;
}

/* Number of ascii digits needed to print any unsigned long in decimal.
 * There are approximately 10 bits for every 3 decimal digits.
 * So from bits to digits the formula is roundup((Number of bits)/10) * 3.
 * For common sizes of integers this works out to:
 *  2bytes -->  6 ascii estimate  -> 65536  (5 real)
 *  4bytes --> 12 ascii estimated -> 4294967296 (10 real)
 *  8bytes --> 21 ascii estimated -> 18446744073709551616 (20 real)
 * 16bytes --> 39 ascii estimated -> 340282366920938463463374607431768211456 (39 real)
 */
#define ULONG_DIGITS (((WIDTHOF(unsigned long) + 9)/10)*3)

#if HAVE_SYS_CAPABILITY_H
static inline bool maps_lower_root(int cap, int ranges, const struct map_range *mappings)
{
	int idx;
	const struct map_range *mapping;

	if (cap != CAP_SETUID)
		return false;

	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		if (mapping->lower == 0)
			return true;
	}

	return false;
}
#endif

/*
 * The ruid refers to the caller's uid and is used to reset the effective uid
 * back to the callers real uid.
 * This clutch mainly exists for setuid-based new{g,u}idmap binaries that are
 * called in contexts where all capabilities other than the necessary
 * CAP_SET{G,U}ID capabilities are dropped. Since the kernel will require
 * assurance that the caller holds CAP_SYS_ADMIN over the target user namespace
 * the only way it can confirm is in this case is if the effective uid is
 * equivalent to the uid owning the target user namespace.
 * Note, we only support this when a) new{g,u}idmap is not called by root and
 * b) if the caller's uid and the uid retrieved via system appropriate means
 * (shadow file or other) are identical. Specifically, this does not support
 * when the root user calls the new{g,u}idmap binary for an unprivileged user.
 * If this is wanted: use file capabilities!
 */
void write_mapping(int proc_dir_fd, int ranges, const struct map_range *mappings,
	const char *map_file, uid_t ruid)
{
	int idx;
	const struct map_range *mapping;
	size_t bufsize;
	char *buf, *pos, *end;
	int fd;

#if HAVE_SYS_CAPABILITY_H
	int cap;
	struct __user_cap_header_struct hdr = {_LINUX_CAPABILITY_VERSION_3, 0};
	struct __user_cap_data_struct data[2] = {{0}};

	if (strcmp(map_file, "uid_map") == 0) {
		cap = CAP_SETUID;
	} else if (strcmp(map_file, "gid_map") == 0) {
		cap = CAP_SETGID;
	} else {
		fprintf(log_get_logfd(), _("%s: Invalid map file %s specified\n"), log_get_progname(), map_file);
		exit(EXIT_FAILURE);
	}

	/* Align setuid- and fscaps-based new{g,u}idmap behavior. */
	if (geteuid() == 0 && geteuid() != ruid) {
		if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0) {
			fprintf(log_get_logfd(), _("%s: Could not prctl(PR_SET_KEEPCAPS)\n"), log_get_progname());
			exit(EXIT_FAILURE);
		}

		if (seteuid(ruid) < 0) {
			fprintf(log_get_logfd(), _("%s: Could not seteuid to %d\n"), log_get_progname(), ruid);
			exit(EXIT_FAILURE);
		}
	}

	/* Lockdown new{g,u}idmap by dropping all unneeded capabilities. */
	bzero(data, sizeof(data));
	data[0].effective = CAP_TO_MASK(cap);
	/*
	 * When uid 0 from the ancestor userns is supposed to be mapped into
	 * the child userns we need to retain CAP_SETFCAP.
	 */
	if (maps_lower_root(cap, ranges, mappings))
		data[0].effective |= CAP_TO_MASK(CAP_SETFCAP);
	data[0].permitted = data[0].effective;
	if (capset(&hdr, data) < 0) {
		fprintf(log_get_logfd(), _("%s: Could not set caps\n"), log_get_progname());
		exit(EXIT_FAILURE);
	}
#endif

	bufsize = (ULONG_DIGITS + 1) * 3 * ranges + 1;
	pos = buf = XMALLOC(bufsize, char);
	end = buf + bufsize;

	/* Build the mapping command */
	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		/* Append this range to the string that will be written */
		pos = stpeprintf(pos, end, "%lu %lu %lu\n",
		                 mapping->upper,
		                 mapping->lower,
		                 mapping->count);
	}
	if (pos == end || pos == NULL) {
		fprintf(log_get_logfd(), _("%s: stpeprintf failed!\n"), log_get_progname());
		exit(EXIT_FAILURE);
	}

	/* Write the mapping to the mapping file */
	fd = openat(proc_dir_fd, map_file, O_WRONLY);
	if (fd < 0) {
		fprintf(log_get_logfd(), _("%s: open of %s failed: %s\n"),
			log_get_progname(), map_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (write_full(fd, buf, pos - buf) == -1) {
		fprintf(log_get_logfd(), _("%s: write to %s failed: %s\n"),
			log_get_progname(), map_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (close(fd) != 0 && errno != EINTR) {
		fprintf(log_get_logfd(), _("%s: closing %s failed: %s\n"),
			log_get_progname(), map_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	free(buf);
}
