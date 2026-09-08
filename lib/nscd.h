#ifndef _NSCD_H_
#define _NSCD_H_

/*
 * nscd_flush_cache - flush specified service buffer in nscd cache
 */
#ifdef	USE_NSCD
extern int nscd_flush_cache (const char *service);
#else
static inline int
nscd_flush_cache(MAYBE_UNUSED int _1)
{
	return 0;
}
#endif

#endif
