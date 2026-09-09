/*
 * SPDX-FileCopyrightText: 2013 Eric Biederman
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <linux/capability.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "alloc/calloc.h"
#include "alloc/malloc.h"
#include "atoi/a2i.h"
#include "attr.h"
#include "idmapping.h"
#include "io/fprintf.h"
#include "prototypes.h"
#include "shadowlog.h"
#include "sizeof.h"
#include "string/sprintf/seprintf.h"
#include "string/strcmp/streq.h"


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

	mappings = calloc_T(ranges, struct map_range);
	if (!mappings) {
		fprintf(log_get_logfd(), _( "%s: Memory allocation failure\n"),
			log_get_progname());
		return NULL;
	}

	/* Gather up the ranges from the command line */
	m = mappings;
	for (int i = 0; i < ranges * 3; i+=3, m++) {
		if (a2i(unsigned long, &m->upper, argv[i + 0], NULL, 0, 0, UINT_MAX - 1) == -1)
		{
			if (errno == ERANGE)
				fprintf(log_get_logfd(), _( "%s: subuid overflow detected.\n"), log_get_progname());
			free(mappings);
			return NULL;
		}
		if (a2i(unsigned long, &m->lower, argv[i + 1], NULL, 0, 0, UINT_MAX - 1) == -1)
		{
			if (errno == ERANGE)
				fprintf(log_get_logfd(), _( "%s: subuid overflow detected.\n"), log_get_progname());
			free(mappings);
			return NULL;
		}
		if (a2i(unsigned long, &m->count, argv[i + 2], NULL, 0, 1, UINT_MAX - MAX(m->lower, m->upper)) == -1)
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

static inline int my_capset(cap_user_header_t hdrp, const cap_user_data_t datap)
{
	return syscall(SYS_capset, hdrp, datap);
}

void write_mapping(int proc_dir_fd, int ranges, const struct map_range *mappings,
	const char *map_file, MAYBE_UNUSED uid_t ruid)
{
	int idx;
	const struct map_range *mapping;
	size_t bufsize;
	char *buf, *pos, *end;
	int fd;

	int cap;
	struct __user_cap_header_struct hdr = {_LINUX_CAPABILITY_VERSION_3, 0};
	struct __user_cap_data_struct data[2] = {{0}};

	if (streq(map_file, "uid_map")) {
		cap = CAP_SETUID;
	} else if (streq(map_file, "gid_map")) {
		cap = CAP_SETGID;
	} else {
		fprintf(log_get_logfd(), _("%s: Invalid map file %s specified\n"), log_get_progname(), map_file);
		exit(EXIT_FAILURE);
	}

	/*
	 * In order to write an idmap, we need to "have capabiilties
	 * in the *target* userns" -- this is ns_capable or file_ns_capable
	 * in the Linux sources.  Depending on exactly what mapping we're
	 * writing, this is CAP_SYS_ADMIN and/or CAP_SETFCAP.  We can
	 * have these capabilities in one of two ways: we can hold them
	 * on our own effective capability set or we can have
	 * euid == the namespace owner.  There is no special case for euid == 0
	 * in that check/
	 *
	 * It's rather popular to run inside an outer "container" with
	 * CAP_SYS_ADMIN and CAP_SETFCAP removed from its
	 * bounding set and to have newuidmap be setuid root.
	 *
	 * Usually we will be run by the namespace owner, so ruid will
	 * equal the target namespace owner.  Setting euid back to ruid
	 * will restore our capabilities over the target in case we
	 * don't have CAP_SYS_ADMIN.  It also hardens us a bit in that we
	 * should really not be using our euid==0 to accidentally have capabilites
	 * in the target namespace if that namespace is owned by uid 0 and our
	 * ruid != 0.  (Note that this is just hardening -- we are quite careful
	 * not to make this mistake in the first place.)
	 *
	 * This also serves to align setuid- and fscaps-based
	 * new{g,u}idmap behavior.
	 */
	if (geteuid() == 0 && geteuid() != ruid) {
		if (prctl(PR_SET_KEEPCAPS, 1L) == -1) {
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
	if (my_capset(&hdr, data) < 0) {
		fprintf(log_get_logfd(), _("%s: Could not set caps\n"), log_get_progname());
		exit(EXIT_FAILURE);
	}

	bufsize = (ULONG_DIGITS + 1) * 3 * ranges + 1;
	pos = buf = xmalloc_T(bufsize, char);
	end = buf + bufsize;

	/* Build the mapping command */
	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		/* Append this range to the string that will be written */
		pos = seprintf(pos, end, "%lu %lu %lu\n",
		                 mapping->upper,
		                 mapping->lower,
		                 mapping->count);
	}
	if (pos == end || pos == NULL) {
		fprintf(log_get_logfd(), _("%s: seprintf failed!\n"), log_get_progname());
		exit(EXIT_FAILURE);
	}

	/* Write the mapping to the mapping file */
	fd = openat(proc_dir_fd, map_file, O_WRONLY);
	if (fd < 0) {
		fprinte(log_get_logfd(), _("%s: open of %s failed"),
			log_get_progname(), map_file);
		exit(EXIT_FAILURE);
	}
	if (write_full(fd, buf, pos - buf) == -1) {
		fprinte(log_get_logfd(), _("%s: write to %s failed"),
			log_get_progname(), map_file);
		exit(EXIT_FAILURE);
	}
	if (close(fd) != 0 && errno != EINTR) {
		fprinte(log_get_logfd(), _("%s: closing %s failed"),
			log_get_progname(), map_file);
		exit(EXIT_FAILURE);
	}
	free(buf);
}
