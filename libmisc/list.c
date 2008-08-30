/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Removed duplicated code from gpmain.c, useradd.c, userdel.c and
   usermod.c.  --marekm */

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
/*
 * add_list - add a member to a list of group members
 *
 *	the array of member names is searched for the new member
 *	name, and if not present it is added to a freshly allocated
 *	list of users.
 */
char **add_list (char **list, const char *member)
{
	int i;
	char **tmp;

	assert (NULL != member);

	/*
	 * Scan the list for the new name.  Return the original list
	 * pointer if it is present.
	 */

	for (i = 0; list[i] != (char *) 0; i++) {
		if (strcmp (list[i], member) == 0) {
			return list;
		}
	}

	/*
	 * Allocate a new list pointer large enough to hold all the
	 * old entries, and the new entries as well.
	 */

	tmp = (char **) xmalloc ((i + 2) * sizeof member);

	/*
	 * Copy the original list to the new list, then append the
	 * new member and NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = 0; list[i] != (char *) 0; i++) {
		tmp[i] = list[i];
	}

	tmp[i] = xstrdup (member);
	tmp[i+1] = (char *) 0;

	return tmp;
}

/*
 * del_list - delete a member from a list of group members
 *
 *	the array of member names is searched for the old member
 *	name, and if present it is deleted from a freshly allocated
 *	list of users.
 */

char **del_list (char **list, const char *member)
{
	int i, j;
	char **tmp;

	assert (NULL != member);

	/*
	 * Scan the list for the old name.  Return the original list
	 * pointer if it is not present.
	 */

	for (i = j = 0; list[i] != (char *) 0; i++) {
		if (strcmp (list[i], member) != 0) {
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

	tmp = (char **) xmalloc ((j + 1) * sizeof member);

	/*
	 * Copy the original list except the deleted members to the
	 * new list, then NULL terminate the result.  This new list
	 * is returned to the invoker.
	 */

	for (i = j = 0; list[i] != (char *) 0; i++) {
		if (strcmp (list[i], member) != 0) {
			tmp[j] = list[i];
			j++;
		}
	}

	tmp[j] = (char *) 0;

	return tmp;
}

char **dup_list (char *const *list)
{
	int i;
	char **tmp;

	for (i = 0; NULL != list[i]; i++);

	tmp = (char **) xmalloc ((i + 1) * sizeof (char *));

	i = 0;
	while (NULL != *list) {
		tmp[i] = xstrdup (*list);
		i++;
		list++;
	}

	tmp[i] = (char *) 0;
	return tmp;
}

bool is_on_list (char *const *list, const char *member)
{
	assert (NULL != member);

	while (NULL != *list) {
		if (strcmp (*list, member) == 0) {
			return true;
		}
		list++;
	}

	return false;
}

/*
 * comma_to_list - convert comma-separated list to (char *) array
 */

char **comma_to_list (const char *comma)
{
	char *members;
	char **array;
	int i;
	char *cp, *cp2;

	assert (NULL != comma);

	/*
	 * Make a copy since we are going to be modifying the list
	 */

	members = xstrdup (comma);

	/*
	 * Count the number of commas in the list
	 */

	for (cp = members, i = 0;; i++) {
		cp2 = strchr (cp, ',');
		if (NULL != cp2) {
			cp = cp2 + 1;
		} else {
			break;
		}
	}

	/*
	 * Add 2 - one for the ending NULL, the other for the last item
	 */

	i += 2;

	/*
	 * Allocate the array we're going to store the pointers into.
	 */

	array = (char **) xmalloc (sizeof (char *) * i);

	/*
	 * Empty list is special - 0 members, not 1 empty member.  --marekm
	 */

	if ('\0' == *members) {
		*array = (char *) 0;
		return array;
	}

	/*
	 * Now go walk that list all over again, this time building the
	 * array of pointers.
	 */

	for (cp = members, i = 0;; i++) {
		array[i] = cp;
		cp2 = strchr (cp, ',');
		if (NULL != cp2) {
			*cp2 = '\0';
			cp2++;
			cp = cp2;
		} else {
			array[i + 1] = (char *) 0;
			break;
		}
	}

	/*
	 * Return the new array of pointers
	 */

	return array;
}

