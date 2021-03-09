#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <subid.h>
#include <string.h>

enum subid_retval shadow_subid_has_any_range(const char *owner, enum subid_type t)
{
	if (strcmp(owner, "unknown") == 0)
		return SUBID_RV_UNKOWN_USER;
	if (t == ID_TYPE_UID)
		return strcmp(owner, "user1") == 0;
	if (strcmp(owner, "group1") == 0)
		return SUBID_RV_SUCCESS;
	return SUBID_RV_EPERM;
}

enum subid_retval shadow_subid_has_range(const char *owner, unsigned long start, unsigned long count, enum subid_type t)
{
	if (strcmp(owner, "unknown") == 0)
		return SUBID_RV_UNKOWN_USER;
	if (t == ID_TYPE_UID)
		if (strcmp(owner, "user1") != 0)
			return SUBID_RV_EPERM;
	if (t == ID_TYPE_GID)
		if (strcmp(owner, "group1") != 0)
			return SUBID_RV_EPERM;
	if (start < 100000)
		return SUBID_RV_EPERM;
	if (start +count >= 165536)
		return SUBID_RV_EPERM;
	return SUBID_RV_SUCCESS;
}

int shadow_subid_find_subid_owners(unsigned long id, uid_t **uids, enum subid_type id_type)
{
	if (id < 100000 || id >= 165536)
		return 0;
	*uids = malloc(sizeof(uid_t));
	if (!*uids)
		return -1;
	*uids[0] = (uid_t)1000;
	return 1;
}

struct subordinate_range **shadow_subid_nss_list_owner_ranges(const char *owner, enum subid_type id_type)
{
	struct subordinate_range **ranges;

	if (strcmp(owner, "user1") != 0)
		return NULL;
	ranges = (struct subordinate_range **)malloc(2 * sizeof(struct subordinate_range *));
	if (!*ranges)
		return NULL;
	ranges[0] = (struct subordinate_range *)malloc(sizeof(struct subordinate_range));
	if (!ranges[0]) {
		free(*ranges);
		return NULL;
	}
	ranges[0]->owner = strdup("user1");
	ranges[0]->start = 100000;
	ranges[0]->count = 65536;

	ranges[1] = NULL;

	return ranges;
}
