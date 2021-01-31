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

#define SUBID_NFIELDS 3
#endif
