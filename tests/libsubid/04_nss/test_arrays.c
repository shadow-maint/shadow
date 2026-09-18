/* SPDX-License-Identifier: BSD-3-Clause */

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "memory/memcmp/memeq.h"
#include "string/strcmp/streq.h"
#include "subid.h"

static int  failures;
static int  (*zzz_outstanding)(void);

static const char *
type_name(enum subid_type type)
{
	return type == ID_TYPE_UID ? "uid" : "gid";
}

static struct subid_range *
get_ranges(enum subid_type type, const char *owner, int *n)
{
	if (type == ID_TYPE_UID)
		return subid_get_uid_ranges(owner, n);
	return subid_get_gid_ranges(owner, n);
}

static uid_t *
get_owners(enum subid_type type, uid_t id, int *n)
{
	if (type == ID_TYPE_UID)
		return subid_get_uid_owners(id, n);
	return subid_get_gid_owners(id, n);
}

// check_released: the module has released every array it handed out
static void
check_released(enum subid_type type, const char *what)
{
	if (zzz_outstanding() != 0) {
		printf("FAIL %s %s: %d module arrays not released\n",
		       type_name(type), what, zzz_outstanding());
		failures++;
	}
}

// check_ranges: an array of the 'n' ranges in 'want'
static void
check_ranges(enum subid_type type, const char *owner, int n,
    const struct subid_range want[n])
{
	int                 got;
	struct subid_range  *r;

	r = get_ranges(type, owner, &got);
	check_released(type, owner);
	if (r == NULL) {
		printf("FAIL %s %s: NULL; want %d ranges\n",
		       type_name(type), owner, n);
		failures++;
	} else if (got != n) {
		printf("FAIL %s %s: %d ranges; want %d\n",
		       type_name(type), owner, got, n);
		failures++;
	} else if (!memeq(r, want, n * sizeof(r[0]))) {
		printf("FAIL %s %s: wrong ranges\n", type_name(type), owner);
		failures++;
	}
	subid_free(r);
}

// check_fail: no array, and errno 'want'
static void
check_fail(enum subid_type type, const char *owner, int want)
{
	int                 err;
	int                 got;
	struct subid_range  *r;

	r = get_ranges(type, owner, &got);
	err = errno;
	check_released(type, owner);
	if (r != NULL) {
		printf("FAIL %s %s: %d ranges; want NULL\n",
		       type_name(type), owner, got);
		failures++;
	} else if (err != want) {
		printf("FAIL %s %s: errno %d; want %d\n",
		       type_name(type), owner, err, want);
		failures++;
	}
	subid_free(r);
}

// check_owners: an array of 'n' owners
static void
check_owners(enum subid_type type, uid_t id, int n)
{
	int    got;
	uid_t  *uids;

	uids = get_owners(type, id, &got);
	check_released(type, "owners");
	if (uids == NULL) {
		printf("FAIL %s owners %ju: NULL; want %d\n",
		       type_name(type), (uintmax_t) id, n);
		failures++;
	} else if (got != n) {
		printf("FAIL %s owners %ju: %d; want %d\n",
		       type_name(type), (uintmax_t) id, got, n);
		failures++;
	}
	subid_free(uids);
}

int
main(int, char *argv[])
{
	void                             *h;
	static const struct subid_range  none[0];
	static const struct subid_range  one[1] = {{100000, 65536}};
	static const struct subid_range  two[2] = {{100000, 65536},
	                                          {300000, 65536}};

	if (!subid_init("test_arrays", stderr))
		return 1;
	h = dlopen("libsubid_zzz.so", RTLD_LAZY);
	if (h == NULL) {
		printf("FAIL: %s\n", dlerror());
		return 1;
	}
	zzz_outstanding = dlsym(h, "zzz_outstanding");
	if (zzz_outstanding == NULL) {
		printf("FAIL: %s\n", dlerror());
		return 1;
	}

	if (argv[1] != NULL && streq(argv[1], "outage")) {
		check_fail(ID_TYPE_UID, "root", EAGAIN);
		check_fail(ID_TYPE_UID, "0", EAGAIN);
		check_fail(ID_TYPE_GID, "root", EAGAIN);
		return failures != 0;
	}

	check_ranges(ID_TYPE_UID, "user1", 1, one);
	check_ranges(ID_TYPE_UID, "multi", 2, two);
	check_ranges(ID_TYPE_UID, "user2", 0, none);
	check_ranges(ID_TYPE_UID, "emptyarr", 0, none);
	check_fail(ID_TYPE_UID, "unknown", ENOENT);
	check_fail(ID_TYPE_UID, "conn", EAGAIN);
	check_fail(ID_TYPE_UID, "error", EIO);
	check_ranges(ID_TYPE_GID, "group1", 1, one);
	check_ranges(ID_TYPE_GID, "multi", 2, two);
	check_ranges(ID_TYPE_GID, "user1", 0, none);
	check_fail(ID_TYPE_GID, "unknown", ENOENT);

	check_owners(ID_TYPE_UID, 100000, 1);
	check_owners(ID_TYPE_UID, 5, 0);
	check_owners(ID_TYPE_GID, 100000, 1);
	check_owners(ID_TYPE_GID, 5, 0);

	return failures != 0;
}
