#include <config.h>

#ident "$Id$"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if HAVE_SYS_RANDOM_H
#include <sys/random.h>
#endif
#include "bit.h"
#include "prototypes.h"
#include "shadowlog.h"


/*
 * Return a uniformly-distributed CS random u_long value.
 */
unsigned long
csrand(void)
{
	FILE           *fp;
	unsigned long  r;

#ifdef HAVE_GETENTROPY
	/* getentropy may exist but lack kernel support.  */
	if (getentropy(&r, sizeof(r)) == 0)
		return r;
#endif

#ifdef HAVE_GETRANDOM
	/* Likewise getrandom.  */
	if (getrandom(&r, sizeof(r), 0) == sizeof(r))
		return r;
#endif

#ifdef HAVE_ARC4RANDOM_BUF
	/* arc4random_buf can never fail.  */
	arc4random_buf(&r, sizeof(r));
	return r;
#endif

	/* Use /dev/urandom as a last resort.  */
	fp = fopen("/dev/urandom", "r");
	if (NULL == fp) {
		goto fail;
	}

	if (fread(&r, sizeof(r), 1, fp) != 1) {
		fclose(fp);
		goto fail;
	}

	fclose(fp);
	return r;

fail:
	fprintf(log_get_logfd(), _("Unable to obtain random bytes.\n"));
	exit(1);
}
