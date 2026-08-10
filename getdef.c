/*
 * Copyright 1991, 1992, John F. Haugh II and Chip Rosenthal
 * All rights reserved.
 *
 * Permission is granted to copy and create derivative works for any
 * non-commercial purpose, provided this copyright notice is preserved
 * in all copies of source code, or included in human readable form
 * and conspicuously displayed on all copies of object code or
 * distribution media.
 *
 * This software is provided on an AS-IS basis and the author makes
 * no warrantee of any kind.
 */

#ifndef lint
static	char	sccsid[] = "@(#)getdef.c	3.7	13:02:29	27 Jul 1992";
#endif

#include <stdio.h>
#include <ctype.h>
#ifndef BSD
# include <string.h>
#else
# include <strings.h>
#endif
#include "config.h"

#ifdef	USE_SYSLOG
#include <syslog.h>

#ifndef	LOG_WARN
#define	LOG_WARN	LOG_WARNING
#endif
#endif

/*
 * A configuration item definition.
 */

struct itemdef {
	char *name;		/* name of the item			*/
	char *value;		/* value given, or NULL if no value	*/
};

/*
 * This list *must* be sorted by the "name" member.
 */

#define NUMDEFS	(sizeof(def_table)/sizeof(def_table[0]))
struct itemdef def_table[] = {
	{ "CONSOLE",			NULL },
	{ "DIALUPS_CHECK_ENAB",		NULL },
	{ "ENV_HZ",			NULL },
	{ "ENV_PATH" ,			NULL },
	{ "ENV_SUPATH",			NULL },
	{ "ENV_TZ",			NULL },
	{ "ERASECHAR",			NULL },
	{ "FAILLOG_ENAB",		NULL },
	{ "FAIL_DELAY",			NULL },
	{ "FTMP_FILE",			NULL },
	{ "HUSHLOGIN_FILE",		NULL },
	{ "ISSUE_FILE_ENAB",		NULL },
	{ "KILLCHAR",			NULL },
	{ "LASTLOG_ENAB",		NULL },
	{ "LOG_UNKFAIL_ENAB",		NULL },
	{ "MAIL_CHECK_ENAB",		NULL },
	{ "MAIL_DIR",			NULL },
	{ "MAIL_FILE",			NULL },
	{ "MOTD_FILE",			NULL },
	{ "NOLOGINS_FILE",		NULL },
	{ "NOLOGIN_STR",		NULL },
	{ "OBSCURE_CHECKS_ENAB",	NULL },
	{ "PASS_MAX_DAYS",		NULL },
	{ "PASS_MIN_DAYS",		NULL },
	{ "PASS_MIN_LEN",		NULL },
	{ "PASS_WARN_AGE",		NULL },
	{ "PORTTIME_CHECKS_ENAB",	NULL },
	{ "QUOTAS_ENAB",		NULL },
	{ "SULOG_FILE",			NULL },
	{ "SU_NAME",			NULL },
	{ "SYSLOG_SG_ENAB",		NULL },
	{ "SYSLOG_SU_ENAB",		NULL },
	{ "TTYGROUP",			NULL },
	{ "TTYPERM",			NULL },
	{ "TTYTYPE_FILE",		NULL },
	{ "ULIMIT",			NULL },
	{ "UMASK",			NULL },
};

static char def_fname[] = LOGINDEFS;	/* login config defs file	*/
static int def_loaded = 0;		/* are defs already loaded?	*/

extern long strtol();

static struct itemdef *def_find();
static void def_load();


/*
 * getdef_str - get string value from table of definitions.
 *
 * Return point to static data for specified item, or NULL if item is not
 * defined.  First time invoked, will load definitions from the file.
 */

char *
getdef_str(item)
char *item;
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	return ((d = def_find(item)) == NULL ? (char *)NULL : d->value);
}


/*
 * getdef_bool - get boolean value from table of definitions.
 *
 * Return TRUE if specified item is defined as "yes", else FALSE.
 */

int
getdef_bool(item)
char *item;
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return 0;

	return (strcmp(d->value, "yes") == 0);
}


/*
 * getdef_num - get numerical value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

int
getdef_num(item, dflt)
char *item;
int dflt;
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return dflt;

	return (int) strtol(d->value, (char **)NULL, 0);
}


/*
 * getdef_long - get long integer value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

long
getdef_long(item, dflt)
char *item;
long dflt;
{
	struct itemdef *d;

	if (!def_loaded)
		def_load();

	if ((d = def_find(item)) == NULL || d->value == NULL)
		return dflt;

	return strtol(d->value, (char **)NULL, 0);
}

/*
 * def_find - locate named item in table
 *
 * Search through a sorted table of configurable items to locate the
 * specified configuration option.
 */

static struct itemdef *
def_find(name)
char *name;
{
	int min, max, curr, n;

	/*
	 * Invariant - desired item in range [min:max].
	 */

	min = 0;
	max = NUMDEFS-1;

	/*
	 * Binary search into the table.  Relies on the items being
	 * sorted by name.
	 */

	while (min <= max) {
		curr = (min+max)/2;

		if (! (n = strcmp(def_table[curr].name, name)))
			return &def_table[curr];

		if (n < 0)
			min = curr+1;
		else
			max = curr-1;
	}

	/*
	 * Item was never found.
	 */

	fprintf(stderr, "configuration error - unknown item '%s' (notify administrator)\r\n", name);
#ifdef USE_SYSLOG
	syslog(LOG_CRIT, "unknown configuration item `%s'", name);
#endif
	return (struct itemdef *) NULL;
}

/*
 * def_load - load configuration table
 *
 * Loads the user-configured options from the default configuration file
 */

static void
def_load()
{
	int i;
	FILE *fp;
	struct itemdef *d;
	char buf[BUFSIZ], *name, *value, *s;

#ifdef CKDEFS

	/*
	 * Set this flag early so the errors will be reported only
	 * during testing.
	 */

	++def_loaded;
#endif

	/*
	 * Open the configuration definitions file.
	 */

	if ((fp = fopen(def_fname, "r")) == NULL) {
#ifdef USE_SYSLOG
		extern int errno;
		extern char *sys_errlist[];

		syslog(LOG_CRIT, "cannot open login definitions %s [%s]",
			def_fname, sys_errlist[errno]);
#endif
		return;
	}

	/*
	 * Go through all of the lines in the file.
	 */

	while (fgets(buf, sizeof(buf), fp) != NULL) {

		/*
		 * Trim trailing whitespace.
		 */

		for (i = strlen(buf)-1 ; i >= 0 ; --i) {
			if (!isspace(buf[i]))
				break;
		}
		buf[++i] = '\0';

		/*
		 * Break the line into two fields.
		 */

		name = buf + strspn(buf, " \t");	/* first nonwhite */
		if (*name == '\0' || *name == '#')
			continue;			/* comment or empty */

		s = name + strcspn(name, " \t");	/* end of field */
		if (*s == '\0')
			continue;			/* only 1 field?? */

		*s++ = '\0';
		value = s + strspn(s, " \t");		/* next nonwhite */

		/*
		 * Locate the slot to save the value.  If this parameter
		 * is unknown then "def_find" will print an err message.
		 */

		if ((d = def_find(name)) == NULL)
			continue;

		/*
		 * Save off the value.
		 */

		if ((d->value = strdup(value)) == NULL) {
			if (! def_loaded)
				break;

			fputs("Could not allocate space for config info.\r\n", stderr);
#ifndef CKDEFS
#ifdef USE_SYSLOG
			syslog(LOG_ERR, "could not allocate space for config info");
#endif
#endif
			break;
		}
	}
	(void) fclose(fp);

	/*
	 * Set the initialized flag.
	 */

	++def_loaded;
}

#ifdef CKDEFS
main(argc, argv)
int	argc;
char	**argv;
{
	int i;
	char *cp;
	struct itemdef *d;

	def_load ();

	for (i = 0 ; i < NUMDEFS ; ++i) {
		if ((d = def_find(def_table[i].name)) == NULL)
			printf("error - lookup '%s' failed\n", def_table[i].name);
		else
			printf("%4d %-24s %s\n", i+1, d->name, d->value);
	}
	for (i = 1;i < argc;i++) {
		if (cp = getdef_str (argv[1]))
			printf ("%s `%s'\n", argv[1], cp);
		else
			printf ("%s not found\n", argv[1]);
	}
	exit(0);
}
#endif
