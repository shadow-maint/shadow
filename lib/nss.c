#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdatomic.h>

#include "alloc/malloc.h"
#include "prototypes.h"
#include "../libsubid/subid.h"
#include "shadowlog.h"
#include "string/sprintf/snprintf.h"
#include "string/strcmp/strcaseprefix.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strspn/stpspn.h"
#include "string/strtok/stpsep.h"


#define NSSWITCH "/etc/nsswitch.conf"

// NSS plugin handling for subids
// If nsswitch has a line like
//    subid: sss
// then the sss module (libsubid_sss.so) will be consulted for subids.
// If nsswitch has a line specifying multiple databases, like:
//    subid: sss files
// then databases will be consulted in the specified order. The search
// stops as soon as the user is found in a database, even if no subids
// are defined there. For example, if 'sss' knows the user but provides
// no subids, 'files' will not be consulted.
//
// While multiple databases are now supported, the subids are a pretty
// limited resource. Mixing local files with network allocations
// (like sssd) requires careful management. Misconfigurations would
// lead to overlapping ID mappings. Use with caution.

static atomic_flag nss_init_started;
static atomic_bool nss_init_completed;

static struct subid_nss_db *subid_nss_db_head;

bool
nss_is_initialized(void) {
	return atomic_load(&nss_init_completed);
}

static void
nss_exit(void) {
	struct subid_nss_db *current;
	struct subid_nss_db *next;

	if (nss_is_initialized() && subid_nss_db_head) {
		current = subid_nss_db_head;
		while (current) {
			next = current->next;
			if (current->ops) {
				dlclose(current->ops->handle);
				free(current->ops);
			}
			free(current);
			current = next;
		}

		subid_nss_db_head = NULL;
	}
}

static struct subid_nss_ops *
open_and_check_nss_module(const char *libname) {
	void                  *h;
	struct subid_nss_ops  *nss_ops;

	h = dlopen(libname, RTLD_LAZY);
	if (!h) {
		fprintf(log_get_logfd(), "Error opening %s: %s\n", libname, dlerror());
		fprintf(log_get_logfd(), "Using files\n");
		return NULL;
	}

	nss_ops = malloc_T(1, struct subid_nss_ops);
	if (!nss_ops) {
		fprintf(log_get_logfd(), "Failed to allocate memory for subid NSS module %s\n", libname);
		dlclose(h);
		return NULL;
	}

	nss_ops->has_range = dlsym(h, "shadow_subid_has_range");
	if (!nss_ops->has_range) {
		fprintf(log_get_logfd(), "%s did not provide @has_range@\n", libname);
		goto close_lib;
	}
	nss_ops->list_owner_ranges = dlsym(h, "shadow_subid_list_owner_ranges");
	if (!nss_ops->list_owner_ranges) {
		fprintf(log_get_logfd(), "%s did not provide @list_owner_ranges@\n", libname);
		goto close_lib;
	}
	nss_ops->find_subid_owners = dlsym(h, "shadow_subid_find_subid_owners");
	if (!nss_ops->find_subid_owners) {
		fprintf(log_get_logfd(), "%s did not provide @find_subid_owners@\n", libname);
		goto close_lib;
	}
	nss_ops->free = dlsym(h, "shadow_subid_free");
	if (!nss_ops->free) {
		fprintf(log_get_logfd(), "%s did not provide @free@\n", libname);
		goto close_lib;
	}

	nss_ops->handle = h;
	return nss_ops;

close_lib:
	dlclose(h);
	free(nss_ops);
	return NULL;
}

// nsswitch_path is an argument only to support testing.
void
nss_init(const char *nsswitch_path) {
	char                  libname[64];
	char                  *line = NULL;
	char                  *p;
	char                  *token;
	FILE                  *nssfp = NULL;
	size_t                len = 0;
	const char            *delimiters = " \t\n";
	struct subid_nss_db   *new_db;
	struct subid_nss_db   **tail = &subid_nss_db_head;
	struct subid_nss_ops  *ops;

	if (atomic_flag_test_and_set(&nss_init_started)) {
		// Another thread has started nss_init, wait for it to complete
		while (!atomic_load(&nss_init_completed))
			usleep(100);
		return;
	}

	if (!nsswitch_path)
		nsswitch_path = NSSWITCH;

	// read nsswitch.conf to check for a line like:
	//   subid:	sss files
	nssfp = fopen(nsswitch_path, "r");
	if (!nssfp) {
		if (errno != ENOENT)
			fprintf(log_get_logfd(), "Failed opening %s: %m\n", nsswitch_path);

		atomic_store(&nss_init_completed, true);
		return;
	}
	p = NULL;
	while (getline(&line, &len, nssfp) != -1) {
		if (strprefix(line, "#"))
			continue;
		if (strlen(line) < 8)
			continue;
		if (!strcaseprefix(line, "subid:"))
			continue;
		p = &line[6];
		p = stpspn(p, delimiters);
		if (!streq(p, ""))
			break;
		p = NULL;
	}

	if (p == NULL) {
		// Use NULL to indicate the built-in "files" database
		subid_nss_db_head = NULL;
		goto done;
	}

	while (NULL != (token = strsep(&p, delimiters))) {
		if (*token == '\0') {
 			continue;
		}

		if (streq(token, "files")) {
			// Use NULL to indicate the built-in "files" database
			ops = NULL;
		} else {
			if (stprintf_a(libname, "libsubid_%s.so", token) == -1) {
				fprintf(log_get_logfd(), "Subid NSS module name too long: %s\n", token);
				continue;
			}

			ops = open_and_check_nss_module(libname);
			if (!ops) {
				continue;
			}
		}

		new_db = malloc_T(1, struct subid_nss_db);
		if (!new_db) {
			if (ops) {
				dlclose(ops->handle);
				free(ops);
				ops = NULL;
			}

			fprintf(log_get_logfd(), "Failed to allocate memory for subid NSS module %s, skipping\n", token);
			continue;
		}

		new_db->ops = ops;
		new_db->next = NULL;
		*tail = new_db;
		tail = &new_db->next;
	}

	if (subid_nss_db_head == NULL) {
		// No vaild NSS database loaded, using "files" only.
		// NULL indicates the built-in "files" database, so we can continue, but log a warning.
		fprintf(log_get_logfd(), "No usable subid NSS module found, using files\n");
	}

done:
	atomic_store(&nss_init_completed, true);
	free(line);
	if (nssfp) {
		atexit(nss_exit);
		fclose(nssfp);
	}
}

struct subid_nss_db *
get_subid_nss_db(void) {
	nss_init(NULL);
	return subid_nss_db_head;
}
