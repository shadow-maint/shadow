#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "prototypes.h"
#include "../libsubid/subid.h"

#define NSSWITCH "/etc/nsswitch.conf"

// NSS plugin handling for subids
// If nsswitch has a line like
//    subid: sssd
// then sssd will be consulted for subids.  Unlike normal NSS dbs,
// only one db is supported at a time.  That's open to debate, but
// the subids are a pretty limited resource, and local files seem
// bound to step on any other allocations leading to insecure
// conditions.
static bool nss_initialized;

struct subid_nss_ops {
	/*
	 * nss_has_any_range: does a user own any subid range
	 *
	 * @owner: username
	 * @idtype: subuid or subgid
	 *
	 * returns true if @owner has been delegated a @idtype subid range.
	 */
	bool (*has_any_range)(const char *owner, enum subid_type idtype);

	/*
	 * nss_has_range: does a user own a given subid range
	 *
	 * @owner: username
	 * @start: first subid in queried range
	 * @count: number of subids in queried range
	 * @idtype: subuid or subgid
	 *
	 * returns true if @owner has been delegated the @idtype subid range
	 * from @start to @start+@count.
	 */
	bool (*has_range)(const char *owner, unsigned long start, unsigned long count, enum subid_type idtype);

	/*
	 * nss_list_owner_ranges: list the subid ranges delegated to a user.
	 *
	 * @owner - string representing username being queried
	 * @id_type - subuid or subgid
	 *
	 * Returns a NULL-terminated array of struct subordinate_range,
	 * or NULL.  The returned array of struct subordinate_range must be
	 * freed by the caller, if not NULL.
	 */
	struct subordinate_range **(*list_owner_ranges)(const char *owner, enum subid_type id_type);
	/*
	 * nss_find_subid_owners: find uids who own a given subuid or subgid.
	 *
	 * @owner - string representing username being queried
	 * @uids - pointer to an array of uids which will be allocated by
	 *         nss_find_subid_owners()
	 * @id_type - subuid or subgid
	 *
	 * Returns the number of uids which own the subid.  @uids must be freed by
	 * the caller.
	 */
	int (*find_subid_owners)(unsigned long id, uid_t **uids, enum subid_type id_type)
	void *handle;
};

static subid_nss_ops *subid_nss_handle;

void *get_subid_nss_handle() {
	return subid_nss_handle;
}

bool nss_is_initialized() {
	return nss_initialized;
}

void nss_exit() {
	if (nss_initialized && subid_nss_handle) {
		dlclose(subid_nss_handle->handle);
		subid_nss_handle = NULL;
	}
}

// nsswitch_path is an argument only to support testing.
void nss_init(char *nsswitch_path) {
	FILE *nssfp = NULL;
	char *line = NULL, *p, *token;
	size_t len = 0;

	if (nss_initialized)
		return;

	if (!nsswitch_path)
		nsswitch_path = NSSWITCH;

	// read nsswitch.conf to check for a line like:
	//   subid:	files
	nssfp = fopen(nsswitch_path, "r");
	if (!nssfp) {
		fprintf(stderr, "Failed opening %s: %m", nsswitch_path);
		return;
	}
	while ((getline(&line, &len, nssfp)) != -1) {
		if (line[0] == '\0' || line[0] == '#')
			continue;
		if (strncasecmp(line, "subid:", 6) != 0)
			continue;
		p = &line[6];
		while ((*p) && isspace(*p))
			p++;
		for (token = strtok(p, " \n\t");
			token;
			token = strtok(NULL, " \n\t")) {
			char libname[65];
			if (strcmp(token, "files") == 0) {
				subid_nss_handle = NULL;
				goto done;
			}
			if (strlen(token) > 50) {
				fprintf(stderr, "Subid NSS module name too long: %s\n", token);
				fprintf(stderr, "Using files\n");
				subid_nss_handle = NULL;
				goto done;
			}
			snprintf(libname, 64,  "libsubid_%s.so", token);
			subid_nss_handle = dlopen(libname, RTLD_LAZY);
			if (!subid_nss_handle) {
				fprintf(stderr, "Error opening %s: %s\n", libname, dlerror());
				fprintf(stderr, "Using files\n");
				goto done;
			}
			goto done;
		}
		fprintf(stderr, "No usable subid NSS module found, using files\n");
		subid_nss_handle = NULL;
		goto done;
	}

done:
	nss_initialized = true;
	free(line);
	if (nssfp) {
		atexit(nss_exit);
		fclose(nssfp);
	}
}
