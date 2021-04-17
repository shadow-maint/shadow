#include <sys/types.h>
#include <stdbool.h>

#ifndef SUBID_RANGE_DEFINED
#define SUBID_RANGE_DEFINED 1
struct subordinate_range {
	const char *owner;
	unsigned long start;
	unsigned long count;
};

enum subid_type {
	ID_TYPE_UID = 1,
	ID_TYPE_GID = 2
};

enum subid_status {
	SUBID_STATUS_SUCCESS = 0,
	SUBID_STATUS_UNKNOWN_USER = 1,
	SUBID_STATUS_ERROR_CONN = 2,
	SUBID_STATUS_ERROR = 3,
};

/*
 * get_subuid_ranges: return a list of UID ranges for a user
 *
 * @owner: username being queried
 * @ranges: a pointer to a subordinate range ** in which the result will be
 *          returned.
 *
 * returns: number of ranges found, ir < 0 on error.
 */
int get_subuid_ranges(const char *owner, struct subordinate_range ***ranges);

/*
 * get_subgid_ranges: return a list of GID ranges for a user
 *
 * @owner: username being queried
 * @ranges: a pointer to a subordinate range ** in which the result will be
 *          returned.
 *
 * returns: number of ranges found, ir < 0 on error.
 */
int get_subgid_ranges(const char *owner, struct subordinate_range ***ranges);

/*
 * subid_free_ranges: free an array of subordinate_ranges returned by either
 *                    get_subuid_ranges() or get_subgid_ranges().
 *
 * @ranges: the ranges to free
 * @count: the number of ranges in @ranges
 */
void subid_free_ranges(struct subordinate_range **ranges, int count);

/*
 * get_subuid_owners: return a list of uids to which the given uid has been
 *                    delegated.
 *
 * @uid: The subuid being queried
 * @owners: a pointer to an array of uids into which the results are placed.
 *          The returned array must be freed by the caller.
 *
 * Returns the number of uids returned, or < 0 on error.
 */
int get_subuid_owners(uid_t uid, uid_t **owner);

/*
 * get_subgid_owners: return a list of uids to which the given gid has been
 *                    delegated.
 *
 * @uid: The subgid being queried
 * @owners: a pointer to an array of uids into which the results are placed.
 *          The returned array must be freed by the caller.
 *
 * Returns the number of uids returned, or < 0 on error.
 */
int get_subgid_owners(gid_t gid, uid_t **owner);

/*
 * grant_subuid_range: assign a subuid range to a user
 *
 * @range: pointer to a struct subordinate_range detailing the UID range
 *         to allocate.  ->owner must be the username, and ->count must be
 *         filled in.  ->start is ignored, and will contain the start
 *         of the newly allocated range, upon success.
 *
 * Returns true if the delegation succeeded, false otherwise.  If true,
 * then the range from (range->start, range->start + range->count) will
 * be delegated to range->owner.
 */
bool grant_subuid_range(struct subordinate_range *range, bool reuse);

/*
 * grant_subsid_range: assign a subgid range to a user
 *
 * @range: pointer to a struct subordinate_range detailing the GID range
 *         to allocate.  ->owner must be the username, and ->count must be
 *         filled in.  ->start is ignored, and will contain the start
 *         of the newly allocated range, upon success.
 *
 * Returns true if the delegation succeeded, false otherwise.  If true,
 * then the range from (range->start, range->start + range->count) will
 * be delegated to range->owner.
 */
bool grant_subgid_range(struct subordinate_range *range, bool reuse);

/*
 * ungrant_subuid_range: remove a subuid allocation.
 *
 * @range: pointer to a struct subordinate_range detailing the UID allocation
 *         to remove.
 *
 * Returns true if successful, false if it failed, for instance if the
 * delegation did not exist.
 */
bool ungrant_subuid_range(struct subordinate_range *range);

/*
 * ungrant_subuid_range: remove a subgid allocation.
 *
 * @range: pointer to a struct subordinate_range detailing the GID allocation
 *         to remove.
 *
 * Returns true if successful, false if it failed, for instance if the
 * delegation did not exist.
 */
bool ungrant_subgid_range(struct subordinate_range *range);

#define SUBID_NFIELDS 3
#endif
