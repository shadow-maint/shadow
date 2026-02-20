#ifndef _SSSD_H_
#define _SSSD_H_

#include "attr.h"

#define SSSD_DB_PASSWD	0x001
#define SSSD_DB_GROUP	0x002

/*
 * sssd_flush_cache - flush specified service buffer in sssd cache
 */
#ifdef	USE_SSSD
extern int sssd_flush_cache (int dbflags);
#else
static inline int
sssd_flush_cache(MAYBE_UNUSED int unused)
{
	return 0;
}
#endif

#endif

