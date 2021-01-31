#include "subid.h"
#include <stdbool.h>

int get_subuid_ranges(const char *owner, struct subordinate_range ***ranges);
int get_subgid_ranges(const char *owner, struct subordinate_range ***ranges);
void subid_free_ranges(struct subordinate_range **ranges, int count);

int get_subuid_owners(uid_t uid, uid_t **owner);
int get_subgid_owners(gid_t gid, uid_t **owner);

/* range should be pre-allocated with owner and count filled in, start is
 * ignored, can be 0 */
bool grant_subuid_range(struct subordinate_range *range, bool reuse);
bool grant_subgid_range(struct subordinate_range *range, bool reuse);

bool ungrant_subuid_range(struct subordinate_range *range);
bool ungrant_subgid_range(struct subordinate_range *range);
