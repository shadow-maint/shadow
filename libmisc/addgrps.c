#include <config.h>

#ifdef HAVE_SETGROUPS

#include "prototypes.h"
#include "defines.h"

#include <stdio.h>
#include <grp.h>
#include <errno.h>

#include "rcsid.h"
RCSID("$Id: addgrps.c,v 1.5 2001/09/01 04:19:15 kloczek Exp $")

#define SEP ",:"

/*
 * Add groups with names from LIST (separated by commas or colons)
 * to the supplementary group set.  Silently ignore groups which are
 * already there.  Warning: uses strtok().
 */

int
add_groups(const char *list)
{
	GETGROUPS_T *grouplist, *tmp;
	int i, ngroups, added;
	struct group *grp;
	char *token;
	char buf[1024];

	if (strlen(list) >= sizeof(buf)) {
		errno = EINVAL;
		return -1;
	}
	strcpy(buf, list);

	i = 16;
	for (;;) {
		grouplist = malloc(i * sizeof(GETGROUPS_T));
		if (!grouplist)
			return -1;
		ngroups = getgroups(i, grouplist);
		if (i > ngroups)
			break;
		/* not enough room, so try allocating a larger buffer */
		free(grouplist);
		i *= 2;
	}
	if (ngroups < 0) {
		free(grouplist);
		return -1;
	}

	added = 0;
	for (token = strtok(buf, SEP); token; token = strtok(NULL, SEP)) {

		grp = getgrnam(token);
		if (!grp) {
			fprintf(stderr, _("Warning: unknown group %s\n"), token);
			continue;
		}

		for (i = 0; i < ngroups && grouplist[i] != grp->gr_gid; i++)
			;

		if (i < ngroups)
			continue;

		if (ngroups >= sysconf(_SC_NGROUPS_MAX)) {
			fprintf(stderr, _("Warning: too many groups\n"));
			break;
		}
		tmp = realloc(grouplist, (ngroups + 1) * sizeof(GETGROUPS_T));
		if (!tmp) {
			free(grouplist);
			return -1;
		}
		tmp[ngroups++] = grp->gr_gid;
		grouplist = tmp;
		added++;
	}

	if (added)
		return setgroups(ngroups, grouplist);

	return 0;
}
#endif
