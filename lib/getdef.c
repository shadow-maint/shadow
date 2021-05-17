/*
 * Copyright (c) 1991 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 2000, Marek Michałkiewicz
 * Copyright (c) 2002 - 2006, Tomasz Kłoczko
 * Copyright (c) 2007 - 2008, Nicolas François
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

#include <config.h>

#ident "$Id$"

#include "prototypes.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#ifdef USE_ECONF
#include <libeconf.h>
#endif
#include "getdef.h"
/*
 * A configuration item definition.
 */
struct itemdef {
	/*@null@*/const char *name;	/* name of the item                     */
	/*@null@*/char *value;		/* value given, or NULL if no value     */
};

#define PAMDEFS					\
	{"CHFN_AUTH", NULL},			\
	{"CHSH_AUTH", NULL},			\
	{"CRACKLIB_DICTPATH", NULL},		\
	{"ENV_HZ", NULL},			\
	{"ENVIRON_FILE", NULL},			\
	{"ENV_TZ", NULL},			\
	{"FAILLOG_ENAB", NULL},			\
	{"FTMP_FILE", NULL},			\
	{"HMAC_CRYPTO_ALGO", NULL},		\
	{"ISSUE_FILE", NULL},			\
	{"LASTLOG_ENAB", NULL},			\
	{"LOGIN_STRING", NULL},			\
	{"MAIL_CHECK_ENAB", NULL},		\
	{"MOTD_FILE", NULL},			\
	{"NOLOGINS_FILE", NULL},		\
	{"OBSCURE_CHECKS_ENAB", NULL},		\
	{"PASS_ALWAYS_WARN", NULL},		\
	{"PASS_CHANGE_TRIES", NULL},		\
	{"PASS_MAX_LEN", NULL},			\
	{"PASS_MIN_LEN", NULL},			\
	{"PORTTIME_CHECKS_ENAB", NULL},		\
	{"QUOTAS_ENAB", NULL},			\
	{"SU_WHEEL_ONLY", NULL},		\
	{"ULIMIT", NULL},

/*
 * Items used in other tools (util-linux, etc.)
 */
#define FOREIGNDEFS				\
	{"ALWAYS_SET_PATH", NULL},		\
	{"ENV_ROOTPATH", NULL},			\
	{"LOGIN_KEEP_USERNAME", NULL},		\
	{"LOGIN_PLAIN_PROMPT", NULL},		\
	{"MOTD_FIRSTONLY", NULL},		\


#define NUMDEFS	(sizeof(def_table)/sizeof(def_table[0]))
static struct itemdef def_table[] = {
	{"CHFN_RESTRICT", NULL},
	{"CONSOLE_GROUPS", NULL},
	{"CONSOLE", NULL},
	{"CREATE_HOME", NULL},
	{"DEFAULT_HOME", NULL},
	{"ENCRYPT_METHOD", NULL},
	{"ENV_PATH", NULL},
	{"ENV_SUPATH", NULL},
	{"ERASECHAR", NULL},
	{"FAIL_DELAY", NULL},
	{"FAKE_SHELL", NULL},
	{"GID_MAX", NULL},
	{"GID_MIN", NULL},
	{"HOME_MODE", NULL},
	{"HUSHLOGIN_FILE", NULL},
	{"KILLCHAR", NULL},
	{"LASTLOG_UID_MAX", NULL},
	{"LOGIN_RETRIES", NULL},
	{"LOGIN_TIMEOUT", NULL},
	{"LOG_OK_LOGINS", NULL},
	{"LOG_UNKFAIL_ENAB", NULL},
	{"MAIL_DIR", NULL},
	{"MAIL_FILE", NULL},
	{"MAX_MEMBERS_PER_GROUP", NULL},
	{"MD5_CRYPT_ENAB", NULL},
	{"NONEXISTENT", NULL},
	{"PASS_MAX_DAYS", NULL},
	{"PASS_MIN_DAYS", NULL},
	{"PASS_WARN_AGE", NULL},
#ifdef USE_SHA_CRYPT
	{"SHA_CRYPT_MAX_ROUNDS", NULL},
	{"SHA_CRYPT_MIN_ROUNDS", NULL},
#endif
#ifdef USE_BCRYPT
	{"BCRYPT_MAX_ROUNDS", NULL},
	{"BCRYPT_MIN_ROUNDS", NULL},
#endif
#ifdef USE_YESCRYPT
	{"YESCRYPT_COST_FACTOR", NULL},
#endif
	{"SUB_GID_COUNT", NULL},
	{"SUB_GID_MAX", NULL},
	{"SUB_GID_MIN", NULL},
	{"SUB_UID_COUNT", NULL},
	{"SUB_UID_MAX", NULL},
	{"SUB_UID_MIN", NULL},
	{"SULOG_FILE", NULL},
	{"SU_NAME", NULL},
	{"SYS_GID_MAX", NULL},
	{"SYS_GID_MIN", NULL},
	{"SYS_UID_MAX", NULL},
	{"SYS_UID_MIN", NULL},
	{"TTYGROUP", NULL},
	{"TTYPERM", NULL},
	{"TTYTYPE_FILE", NULL},
	{"UID_MAX", NULL},
	{"UID_MIN", NULL},
	{"UMASK", NULL},
	{"USERDEL_CMD", NULL},
	{"USERGROUPS_ENAB", NULL},
#ifndef USE_PAM
	PAMDEFS
#endif
#ifdef USE_SYSLOG
	{"SYSLOG_SG_ENAB", NULL},
	{"SYSLOG_SU_ENAB", NULL},
#endif
#ifdef WITH_TCB
	{"TCB_AUTH_GROUP", NULL},
	{"TCB_SYMLINKS", NULL},
	{"USE_TCB", NULL},
#endif
	{"FORCE_SHADOW", NULL},
	{"GRANT_AUX_GROUP_SUBIDS", NULL},
	{"PREVENT_NO_AUTH", NULL},
	{NULL, NULL}
};

#define NUMKNOWNDEFS	(sizeof(knowndef_table)/sizeof(knowndef_table[0]))
static struct itemdef knowndef_table[] = {
#ifdef USE_PAM
	PAMDEFS
#endif
	FOREIGNDEFS
	{NULL, NULL}
};

#ifdef USE_ECONF
#ifdef VENDORDIR
static const char* vendordir = VENDORDIR;
#else
static const char* vendordir = NULL;
#endif
static const char* sysconfdir = "/etc";
#else
#ifndef LOGINDEFS
#define LOGINDEFS "/etc/login.defs"
#endif

static const char* def_fname = LOGINDEFS;	/* login config defs file       */
#endif
static bool def_loaded = false;		/* are defs already loaded?     */

/* local function prototypes */
static /*@observer@*/ /*@null@*/struct itemdef *def_find (const char *);
static void def_load (void);


/*
 * getdef_str - get string value from table of definitions.
 *
 * Return point to static data for specified item, or NULL if item is not
 * defined.  First time invoked, will load definitions from the file.
 */

/*@observer@*/ /*@null@*/const char *getdef_str (const char *item)
{
	struct itemdef *d;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	return ((NULL == d)? (const char *) NULL : d->value);
}


/*
 * getdef_bool - get boolean value from table of definitions.
 *
 * Return TRUE if specified item is defined as "yes", else FALSE.
 */

bool getdef_bool (const char *item)
{
	struct itemdef *d;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	if ((NULL == d) || (NULL == d->value)) {
		return false;
	}

	return (strcasecmp (d->value, "yes") == 0);
}


/*
 * getdef_num - get numerical value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

int getdef_num (const char *item, int dflt)
{
	struct itemdef *d;
	long val;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	if ((NULL == d) || (NULL == d->value)) {
		return dflt;
	}

	if (   (getlong (d->value, &val) == 0)
	    || (val > INT_MAX)
	    || (val < INT_MIN)) {
		fprintf (shadow_logfd,
		         _("configuration error - cannot parse %s value: '%s'"),
		         item, d->value);
		return dflt;
	}

	return (int) val;
}


/*
 * getdef_unum - get unsigned numerical value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

unsigned int getdef_unum (const char *item, unsigned int dflt)
{
	struct itemdef *d;
	long val;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	if ((NULL == d) || (NULL == d->value)) {
		return dflt;
	}

	if (   (getlong (d->value, &val) == 0)
	    || (val < 0)
	    || (val > INT_MAX)) {
		fprintf (shadow_logfd,
		         _("configuration error - cannot parse %s value: '%s'"),
		         item, d->value);
		return dflt;
	}

	return (unsigned int) val;
}


/*
 * getdef_long - get long integer value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

long getdef_long (const char *item, long dflt)
{
	struct itemdef *d;
	long val;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	if ((NULL == d) || (NULL == d->value)) {
		return dflt;
	}

	if (getlong (d->value, &val) == 0) {
		fprintf (shadow_logfd,
		         _("configuration error - cannot parse %s value: '%s'"),
		         item, d->value);
		return dflt;
	}

	return val;
}

/*
 * getdef_ulong - get unsigned long numerical value from table of definitions
 *
 * Returns numeric value of specified item, else the "dflt" value if
 * the item is not defined.  Octal (leading "0") and hex (leading "0x")
 * values are handled.
 */

unsigned long getdef_ulong (const char *item, unsigned long dflt)
{
	struct itemdef *d;
	unsigned long val;

	if (!def_loaded) {
		def_load ();
	}

	d = def_find (item);
	if ((NULL == d) || (NULL == d->value)) {
		return dflt;
	}

	if (getulong (d->value, &val) == 0) {
		/* FIXME: we should have a getulong */
		fprintf (shadow_logfd,
		         _("configuration error - cannot parse %s value: '%s'"),
		         item, d->value);
		return dflt;
	}

	return val;
}

/*
 * putdef_str - override the value read from /etc/login.defs
 * (also used when loading the initial defaults)
 */

int putdef_str (const char *name, const char *value)
{
	struct itemdef *d;
	char *cp;

	if (!def_loaded) {
		def_load ();
	}

	/*
	 * Locate the slot to save the value.  If this parameter
	 * is unknown then "def_find" will print an err message.
	 */
	d = def_find (name);
	if (NULL == d) {
		return -1;
	}

	/*
	 * Save off the value.
	 */
	cp = strdup (value);
	if (NULL == cp) {
		(void) fputs (_("Could not allocate space for config info.\n"),
		              shadow_logfd);
		SYSLOG ((LOG_ERR, "could not allocate space for config info"));
		return -1;
	}

	if (NULL != d->value) {
		free (d->value);
	}

	d->value = cp;
	return 0;
}


/*
 * def_find - locate named item in table
 *
 * Search through a table of configurable items to locate the
 * specified configuration option.
 */

static /*@observer@*/ /*@null@*/struct itemdef *def_find (const char *name)
{
	struct itemdef *ptr;

	/*
	 * Search into the table.
	 */

	for (ptr = def_table; NULL != ptr->name; ptr++) {
		if (strcmp (ptr->name, name) == 0) {
			return ptr;
		}
	}

	/*
	 * Item was never found.
	 */

	for (ptr = knowndef_table; NULL != ptr->name; ptr++) {
		if (strcmp (ptr->name, name) == 0) {
			goto out;
		}
	}
	fprintf (shadow_logfd,
	         _("configuration error - unknown item '%s' (notify administrator)\n"),
	         name);
	SYSLOG ((LOG_CRIT, "unknown configuration item `%s'", name));

out:
	return (struct itemdef *) NULL;
}

/*
 * setdef_config_file - set the default configuration file path
 *
 * must be called prior to any def* calls.
 */

void setdef_config_file (const char* file)
{
#ifdef USE_ECONF
	size_t len;
	char* cp;

	len = strlen(file) + strlen(sysconfdir) + 2;
	cp = malloc(len);
	if (cp == NULL)
		exit (13);
	snprintf(cp, len, "%s/%s", file, sysconfdir);
	sysconfdir = cp;
#ifdef VENDORDIR
	len = strlen(file) + strlen(vendordir) + 2;
	cp = malloc(len);
	if (cp == NULL)
		exit (13);
	snprintf(cp, len, "%s/%s", file, vendordir);
	vendordir = cp;
#endif
#else
	def_fname = file;
#endif
}

/*
 * def_load - load configuration table
 *
 * Loads the user-configured options from the default configuration file
 */

static void def_load (void)
{
#ifdef USE_ECONF
	econf_file *defs_file = NULL;
	econf_err error;
	char **keys;
	size_t key_number;
#else
	int i;
	FILE *fp;
	char buf[1024], *name, *value, *s;
#endif

	/*
	 * Set the initialized flag.
	 * (do it early to prevent recursion in putdef_str())
	 */
	def_loaded = true;

#ifdef USE_ECONF

	error = econf_readDirs (&defs_file, vendordir, sysconfdir, "login", "defs", " \t", "#");
	if (error) {
		if (error == ECONF_NOFILE)
			return;

		SYSLOG ((LOG_CRIT, "cannot open login definitions [%s]",
			econf_errString(error)));
		exit (EXIT_FAILURE);
	}

	if ((error = econf_getKeys(defs_file, NULL, &key_number, &keys))) {
		SYSLOG ((LOG_CRIT, "cannot read login definitions [%s]",
			econf_errString(error)));
		exit (EXIT_FAILURE);
	}

	for (size_t i = 0; i < key_number; i++) {
		char *value;

		econf_getStringValue(defs_file, NULL, keys[i], &value);

		/*
		 * Store the value in def_table.
		 *
		 * Ignore failures to load the login.defs file.
		 * The error was already reported to the user and to
		 * syslog. The tools will just use their default values.
		 */
		(void)putdef_str (keys[i], value);
	}

	econf_free (keys);
	econf_free (defs_file);
#else
	/*
	 * Open the configuration definitions file.
	 */
	fp = fopen (def_fname, "r");
	if (NULL == fp) {
		if (errno == ENOENT)
			return;

		int err = errno;
		SYSLOG ((LOG_CRIT, "cannot open login definitions %s [%s]",
		         def_fname, strerror (err)));
		exit (EXIT_FAILURE);
	}

	/*
	 * Go through all of the lines in the file.
	 */
	while (fgets (buf, (int) sizeof (buf), fp) != NULL) {

		/*
		 * Trim trailing whitespace.
		 */
		for (i = (int) strlen (buf) - 1; i >= 0; --i) {
			if (!isspace (buf[i])) {
				break;
			}
		}
		i++;
		buf[i] = '\0';

		/*
		 * Break the line into two fields.
		 */
		name = buf + strspn (buf, " \t");	/* first nonwhite */
		if (*name == '\0' || *name == '#')
			continue;	/* comment or empty */

		s = name + strcspn (name, " \t");	/* end of field */
		if (*s == '\0')
			continue;	/* only 1 field?? */

		*s++ = '\0';
		value = s + strspn (s, " \"\t");	/* next nonwhite */
		*(value + strcspn (value, "\"")) = '\0';

		/*
		 * Store the value in def_table.
		 *
		 * Ignore failures to load the login.defs file.
		 * The error was already reported to the user and to
		 * syslog. The tools will just use their default values.
		 */
		(void)putdef_str (name, value);
	}

	if (ferror (fp) != 0) {
		int err = errno;
		SYSLOG ((LOG_CRIT, "cannot read login definitions %s [%s]",
		         def_fname, strerror (err)));
		exit (EXIT_FAILURE);
	}

	(void) fclose (fp);
#endif
}


#ifdef CKDEFS
int main (int argc, char **argv)
{
	int i;
	char *cp;
	struct itemdef *d;

	def_load ();

	for (i = 0; i < NUMDEFS; ++i) {
		d = def_find (def_table[i].name);
		if (NULL == d) {
			printf ("error - lookup '%s' failed\n",
			        def_table[i].name);
		} else {
			printf ("%4d %-24s %s\n", i + 1, d->name, d->value);
		}
	}
	for (i = 1; i < argc; i++) {
		cp = getdef_str (argv[1]);
		if (NULL != cp) {
			printf ("%s `%s'\n", argv[1], cp);
		} else {
			printf ("%s not found\n", argv[1]);
		}
	}
	exit (EXIT_SUCCESS);
}
#endif
