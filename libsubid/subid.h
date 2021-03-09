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

enum subid_error {
    SUBID_RV_SUCCESS = 0,
    SUBID_RV_UNKWON_USER = 1,
    SUBID_RV_EPERM = 2,
    SUBID_RV_ERROR = 3,
};

#define SUBID_NFIELDS 3
#endif
