/*
 * SPDX-FileCopyrightText: 1991 - 1994, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <assert.h>

#include "alloc/x/xmalloc.h"
#include "prototypes.h"
#include "defines.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strdup/xstrdup.h"
#include "string/strtok/strsep2ls.h"


/*
 * add_list - add a member to a list of group members
 *
 *	the array of member names is searched for the new member
 *	name, and if not present it is added to a freshly allocated
 *	list of users.
 */
/*@only@*/char **
add_list(/*@returned@*/ /*@only@*/char **list, const char *member)
{
	int i;
	char **tmp;

	assert (NULL != member);
	assert (NULL != list);

	/*
	 * Scan the list for the new name.  Return the original list
	 * pointer if it is present.
	 */

	for (i = 0; list[i] != NULL; i++) {
		if (streq(list[i], member)) {
			return list;
		}
	}

	/*
	 * Allocate a new list pointer large enough to hold all the
	 * old entries, and the new entries as well.
	 */

	tmp = XMALLOC(i + 2, char *);

	/*
	 * Copy the original list to the new list, then append the
	 * new member and NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = 0; list[i] != NULL; i++) {
		tmp[i] = list[i];
	}

	tmp[i] = xstrdup (member);
	tmp[i+1] = NULL;

	return tmp;
}

/*
 * del_list - delete a member from a list of group members
 *
 *	the array of member names is searched for the old member
 *	name, and if present it is deleted from a freshly allocated
 *	list of users.
 */

/*@only@*/char **
del_list(/*@returned@*/ /*@only@*/char **list, const char *member)
{
	int i, j;
	char **tmp;

	assert (NULL != member);
	assert (NULL != list);

	/*
	 * Scan the list for the old name.  Return the original list
	 * pointer if it is not present.
	 */

	for (i = j = 0; list[i] != NULL; i++) {
		if (!streq(list[i], member)) {
			j++;
		}
	}

	if (j == i) {
		return list;
	}

	/*
	 * Allocate a new list pointer large enough to hold all the
	 * old entries.
	 */

	tmp = XMALLOC(j + 1, char *);

	/*
	 * Copy the original list except the deleted members to the
	 * new list, then NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = j = 0; list[i] != NULL; i++) {
		if (!streq(list[i], member)) {
			tmp[j] = list[i];
			j++;
		}
	}

	tmp[j] = NULL;

	return tmp;
}

/*
 * Duplicate a list.
 * The input list is not modified, but in order to allow the use of this
 * function with list of members, the list elements are not enforced to be
 * constant strings here.
 */
/*@only@*/char **
dup_list(char *const *list)
{
	int i;
	char **tmp;

	assert (NULL != list);

	for (i = 0; NULL != list[i]; i++);

	tmp = XMALLOC(i + 1, char *);

	i = 0;
	while (NULL != *list) {
		tmp[i] = xstrdup (*list);
		i++;
		list++;
	}

	tmp[i] = NULL;
	return tmp;
}

/*
 * Check if member is part of the input list
 * The input list is not modified, but in order to allow the use of this
 * function with list of members, the list elements are not enforced to be
 * constant strings here.
 */
bool is_on_list (char *const *list, const char *member)
{
	assert (NULL != member);
	assert (NULL != list);

	while (NULL != *list) {
		if (streq(*list, member)) {
			return true;
		}
		list++;
	}

	return false;
}

/*
 * comma_to_list - convert comma-separated list to (char *) array
 */

/*@only@*/char **
comma_to_list(const char *comma)
{
	char *members;
	char **array;
	size_t  n;

	assert (NULL != comma);

	/*
	 * Make a copy since we are going to be modifying the list
	 */

	members = xstrdup (comma);

	/*
	 * Allocate the array we're going to store the pointers into.
	 * n: number of delimiters + last element + NULL
	 */

	n = strchrcnt(members, ',') + 2;
	array = XMALLOC(n, char *);

	/*
	 * Empty list is special - 0 members, not 1 empty member.  --marekm
	 */

	if (streq(members, "")) {
		*array = NULL;
		free (members);
		return array;
	}

	strsep2ls(members, ",", n, array);

	return array;
}

