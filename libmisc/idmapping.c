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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "prototypes.h"
#include "idmapping.h"
#include <sys/prctl.h>
#if HAVE_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif

struct map_range *get_map_ranges(int ranges, int argc, char **argv)
{
	struct map_range *mappings, *mapping;
	int idx, argidx;

	if (ranges < 0 || argc < 0) {
		fprintf(stderr, "%s: error calculating number of arguments\n", Prog);
		return NULL;
	}

	if (ranges != ((argc + 2) / 3)) {
		fprintf(stderr, "%s: ranges: %u is wrong for argc: %d\n", Prog, ranges, argc);
		return NULL;
	}

	if ((ranges * 3) > argc) {
		fprintf(stderr, "ranges: %u argc: %d\n",
			ranges, argc);
		fprintf(stderr,
			_( "%s: Not enough arguments to form %u mappings\n"),
			Prog, ranges);
		return NULL;
	}

	mappings = calloc(ranges, sizeof(*mappings));
	if (!mappings) {
		fprintf(stderr, _( "%s: Memory allocation failure\n"),
			Prog);
		exit(EXIT_FAILURE);
	}

	/* Gather up the ranges from the command line */
	mapping = mappings;
	for (idx = 0, argidx = 0; idx < ranges; idx++, argidx += 3, mapping++) {
		if (!getulong(argv[argidx + 0], &mapping->upper)) {
			free(mappings);
			return NULL;
		}
		if (!getulong(argv[argidx + 1], &mapping->lower)) {
			free(mappings);
			return NULL;
		}
		if (!getulong(argv[argidx + 2], &mapping->count)) {
			free(mappings);
			return NULL;
		}
		if (ULONG_MAX - mapping->upper <= mapping->count || ULONG_MAX - mapping->lower <= mapping->count) {
			fprintf(stderr, _( "%s: subuid overflow detected.\n"), Prog);
			exit(EXIT_FAILURE);
		}
		if (mapping->upper > UINT_MAX ||
			mapping->lower > UINT_MAX ||
			mapping->count > UINT_MAX)  {
			fprintf(stderr, _( "%s: subuid overflow detected.\n"), Prog);
			exit(EXIT_FAILURE);
		}
		if (mapping->lower + mapping->count > UINT_MAX ||
				mapping->upper + mapping->count > UINT_MAX) {
			fprintf(stderr, _( "%s: subuid overflow detected.\n"), Prog);
			exit(EXIT_FAILURE);
		}
		if (mapping->lower + mapping->count < mapping->lower ||
				mapping->upper + mapping->count < mapping->upper) {
			/* this one really shouldn't be possible given previous checks */
			fprintf(stderr, _( "%s: subuid overflow detected.\n"), Prog);
			exit(EXIT_FAILURE);
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
#define ULONG_DIGITS ((((sizeof(unsigned long) * CHAR_BIT) + 9)/10)*3)

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
void write_mapping(int proc_dir_fd, int ranges, struct map_range *mappings,
	const char *map_file, uid_t ruid)
{
	int idx;
	struct map_range *mapping;
	size_t bufsize;
	char *buf, *pos;
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
		fprintf(stderr, _("%s: Invalid map file %s specified\n"), Prog, map_file);
		exit(EXIT_FAILURE);
	}

	/* Align setuid- and fscaps-based new{g,u}idmap behavior. */
	if (geteuid() != ruid) {
		if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0) {
			fprintf(stderr, _("%s: Could not prctl(PR_SET_KEEPCAPS)\n"), Prog);
			exit(EXIT_FAILURE);
		}

		if (seteuid(ruid) < 0) {
			fprintf(stderr, _("%s: Could not seteuid to %d\n"), Prog, ruid);
			exit(EXIT_FAILURE);
		}
	}

	/* Lockdown new{g,u}idmap by dropping all unneeded capabilities. */
	memset(data, 0, sizeof(data));
	data[0].effective = CAP_TO_MASK(cap);
	data[0].permitted = data[0].effective;
	if (capset(&hdr, data) < 0) {
		fprintf(stderr, _("%s: Could not set caps\n"), Prog);
		exit(EXIT_FAILURE);
	}
#endif

	bufsize = ranges * ((ULONG_DIGITS  + 1) * 3);
	pos = buf = xmalloc(bufsize);

	/* Build the mapping command */
	mapping = mappings;
	for (idx = 0; idx < ranges; idx++, mapping++) {
		/* Append this range to the string that will be written */
		int written = snprintf(pos, bufsize - (pos - buf),
			"%lu %lu %lu\n",
			mapping->upper,
			mapping->lower,
			mapping->count);
		if ((written <= 0) || (written >= (bufsize - (pos - buf)))) {
			fprintf(stderr, _("%s: snprintf failed!\n"), Prog);
			exit(EXIT_FAILURE);
		}
		pos += written;
	}

	/* Write the mapping to the mapping file */
	fd = openat(proc_dir_fd, map_file, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, _("%s: open of %s failed: %s\n"),
			Prog, map_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (write(fd, buf, pos - buf) != (pos - buf)) {
		fprintf(stderr, _("%s: write to %s failed: %s\n"),
			Prog, map_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);
}
