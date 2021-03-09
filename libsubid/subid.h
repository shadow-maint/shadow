#include <sys/types.h>

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

enum subid_retval {
    SUBID_RV_SUCCESS = 0,
    SUBID_RV_UNKOWN_USER = 1,
    SUBID_RV_EPERM = 2,
    SUBID_RV_ERROR = 3,
};

struct subid_nss_ops {
	/*
	 * nss_has_any_range: does a user own any subid range
	 *
	 * @owner: username
	 * @idtype: subuid or subgid
	 *
	 * returns success if @owner has been delegated a @idtype subid range.
	 */
	enum subid_retval (*has_any_range)(const char *owner, enum subid_type idtype);

	/*
	 * nss_has_range: does a user own a given subid range
	 *
	 * @owner: username
	 * @start: first subid in queried range
	 * @count: number of subids in queried range
	 * @idtype: subuid or subgid
	 *
	 * returns success if @owner has been delegated the @idtype subid range
	 * from @start to @start+@count.
	 */
	enum subid_retval (*has_range)(const char *owner, unsigned long start, unsigned long count, enum subid_type idtype);

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
	int (*find_subid_owners)(unsigned long id, uid_t **uids, enum subid_type id_type);

    /* The dlsym handle to close */
	void *handle;
};

#define SUBID_NFIELDS 3
#endif
