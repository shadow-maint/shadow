/*
 * SPDX-FileCopyrightText:  Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>
 * SPDX-FileCopyrightText:  J.T. Conklin <jtc@netbsd.org>
 *
 * SPDX-License-Identifier: Unlicense
 */

/*
 * salt.c - generate a random salt string for crypt()
 *
 * Written by Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>,
 * it is in the public domain.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "defines.h"
#include "getdef.h"
#include "prototypes.h"
#include "shadowlog.h"
#include "string/sprintf/stprintf.h"
#include "string/strcpy/stpecpy.h"
#include "string/strcmp/streq.h"
#include "string/strcpy/strtcat.h"

#undef NDEBUG
#include <assert.h>


#if (defined CRYPT_GENSALT_IMPLEMENTS_AUTO_ENTROPY && \
     CRYPT_GENSALT_IMPLEMENTS_AUTO_ENTROPY)
#define USE_XCRYPT_GENSALT 1
#else
#define USE_XCRYPT_GENSALT 0
#endif

#ifdef USE_BCRYPT
/* Default number of rounds if not explicitly specified.  */
#define B_ROUNDS_DEFAULT 13
/* Minimum number of rounds.  */
#define B_ROUNDS_MIN 4
/* Maximum number of rounds.  */
#define B_ROUNDS_MAX 31
#endif /* USE_BCRYPT */

/* Default number of rounds if not explicitly specified.  */
#define SHA_ROUNDS_DEFAULT 5000
/* Minimum number of rounds.  */
#define SHA_ROUNDS_MIN 1000
/* Maximum number of rounds.  */
#define SHA_ROUNDS_MAX 999999999

#ifdef USE_YESCRYPT
/* Default cost if not explicitly specified.  */
#define Y_COST_DEFAULT 5
/* Minimum cost.  */
#define Y_COST_MIN 1
/* Maximum cost.  */
#define Y_COST_MAX 11
#endif

/* Maximum size of the generated salt string. */
#define GENSALT_SETTING_SIZE 100


enum encrypt_method {
	ENCRYPT_METHOD_SHA256,
	ENCRYPT_METHOD_SHA512,
#ifdef USE_BCRYPT
	ENCRYPT_METHOD_BCRYPT,
#endif
#ifdef USE_YESCRYPT
	ENCRYPT_METHOD_YESCRYPT,
#endif
};


/* local function prototypes */
static unsigned long get_salt_cost(enum encrypt_method m, const long *preferred_cost);
static const char *salt_cost(enum encrypt_method m, unsigned long cost);
static const char *magnum(enum encrypt_method m);
#if !USE_XCRYPT_GENSALT
static /*@observer@*/const char *gensalt(enum encrypt_method m);
#endif /* !USE_XCRYPT_GENSALT */
static /*@observer@*/unsigned long SHA_get_salt_rounds(/*@null@*/const long *prefered_rounds);
static /*@observer@*/const char *SHA_salt_rounds(unsigned long rounds);
#ifdef USE_BCRYPT
static /*@observer@*/unsigned long BCRYPT_get_salt_rounds(/*@null@*/const long *prefered_rounds);
static /*@observer@*/const char *BCRYPT_salt_rounds(unsigned long rounds);
#endif /* USE_BCRYPT */
#ifdef USE_YESCRYPT
static /*@observer@*/unsigned long YESCRYPT_get_salt_cost(/*@null@*/const long *prefered_cost);
static /*@observer@*/const char *YESCRYPT_salt_cost(unsigned long cost);
#endif /* USE_YESCRYPT */


/* Return the the rounds number for the SHA crypt methods. */
static /*@observer@*/unsigned long
SHA_get_salt_rounds(/*@null@*/const long *prefered_rounds)
{
	unsigned long  rounds;

	if (NULL == prefered_rounds) {
		long  min = getdef_long("SHA_CRYPT_MIN_ROUNDS", -1);
		long  max = getdef_long("SHA_CRYPT_MAX_ROUNDS", -1);

		if (-1 == min && -1 == max) {
			rounds = SHA_ROUNDS_DEFAULT;
		} else {
			if (-1 == min)
				min = max;
			if (-1 == max)
				max = min;
			if (min > max)
				max = min;

			rounds = csrand_interval(min, max);
		}
	} else {
		rounds = prefered_rounds[0] ?: SHA_ROUNDS_DEFAULT;
	}

	/* Sanity checks. The libc should also check this, but this
	 * protects against a rounds_prefix overflow. */
	if (rounds < SHA_ROUNDS_MIN)
		rounds = SHA_ROUNDS_MIN;
	if (rounds > SHA_ROUNDS_MAX)
		rounds = SHA_ROUNDS_MAX;

	return rounds;
}

/*
 * Create a salt prefix specifying the rounds number for the SHA crypt methods.
 */
static /*@observer@*/const char *
SHA_salt_rounds(unsigned long rounds)
{
	static char  buf[18];

	/* Nothing to do here if SHA_ROUNDS_DEFAULT is used. */
	if (rounds == SHA_ROUNDS_DEFAULT)
		return strcpy(buf, "");

	/*
	 * We are going to write a maximum of 17 bytes,
	 * plus one byte for the terminator.
	 *    rounds=XXXXXXXXX$
	 *    00000000011111111
	 *    12345678901234567
	 */
	assert(stprintf_a(buf, "rounds=%lu$", rounds) != -1);

	return buf;
}

#ifdef USE_BCRYPT
/* Return the the rounds number for the BCRYPT method. */
static /*@observer@*/unsigned long
BCRYPT_get_salt_rounds(/*@null@*/const long *prefered_rounds)
{
	unsigned long  rounds;

	if (NULL == prefered_rounds) {
		long  min = getdef_long ("BCRYPT_MIN_ROUNDS", -1);
		long  max = getdef_long ("BCRYPT_MAX_ROUNDS", -1);

		if ((-1 == min) && (-1 == max)) {
			rounds = B_ROUNDS_DEFAULT;
		} else {
			if (-1 == min)
				min = max;
			if (-1 == max)
				max = min;
			if (min > max)
				max = min;

			rounds = csrand_interval(min, max);
		}
	} else {
		rounds = prefered_rounds[0] ?: B_ROUNDS_DEFAULT;
	}

	/* Sanity checks. */
	if (rounds < B_ROUNDS_MIN)
		rounds = B_ROUNDS_MIN;

#if USE_XCRYPT_GENSALT
	if (rounds > B_ROUNDS_MAX)
		rounds = B_ROUNDS_MAX;
#else /* USE_XCRYPT_GENSALT */
	/*
	 * Use 19 as an upper bound for now,
	 * because musl doesn't allow rounds >= 20.
	 * If musl ever supports > 20 rounds,
	 * rounds should be set to B_ROUNDS_MAX.
	 */
	if (rounds > 19)
		rounds = 19;
#endif /* USE_XCRYPT_GENSALT */

	return rounds;
}

/*
 * Create a salt prefix specifying the rounds number for the BCRYPT method.
 */
static /*@observer@*/const char *
BCRYPT_salt_rounds(unsigned long rounds)
{
	static char  buf[4];

	/*
	 * We are going to write three bytes,
	 * plus one byte for the terminator.
	 *    XX$
	 *    000
	 *    123
	 */
	assert(stprintf_a(buf, "%2.2lu$", rounds) != -1);

	return buf;
}
#endif /* USE_BCRYPT */

#ifdef USE_YESCRYPT
/* Return the the cost number for the YESCRYPT method. */
static /*@observer@*/unsigned long
YESCRYPT_get_salt_cost(/*@null@*/const long *prefered_cost)
{
	unsigned long cost;

	if (NULL == prefered_cost)
		cost = getdef_num ("YESCRYPT_COST_FACTOR", Y_COST_DEFAULT);
	else
		cost = prefered_cost[0] ?: Y_COST_DEFAULT;

	/* Sanity checks. */
	if (cost < Y_COST_MIN) {
		cost = Y_COST_MIN;
	}

	if (cost > Y_COST_MAX) {
		cost = Y_COST_MAX;
	}

	return cost;
}

/*
 * Create a salt prefix specifying the cost for the YESCRYPT method.
 */
static /*@observer@*/const char *YESCRYPT_salt_cost(unsigned long cost)
{
	char         *p;
	static char  buf[4];

	/*
	 * We are going to write four bytes,
	 * plus one byte for the terminator.
	 *    jXX$
	 *    0000
	 *    1234
	 */

	p = buf;
	p = stpcpy(p, "j");
	if (cost < 3)
		*p++ = 0x36 + cost;
	else if (cost < 6)
		*p++ = 0x34 + cost;
	else
		*p++ = 0x3b + cost;

	p = stpcpy(p, (cost >= 3) ? "T" : "5");
	stpcpy(p, "$");

	return buf;
}
#endif /* USE_YESCRYPT */

static unsigned long
get_salt_cost(enum encrypt_method m, const long *preferred_cost)
{
	switch (m) {
	case ENCRYPT_METHOD_SHA256:
	case ENCRYPT_METHOD_SHA512:
		return SHA_get_salt_rounds(preferred_cost);
#ifdef USE_BCRYPT
	case ENCRYPT_METHOD_BCRYPT:
		return BCRYPT_get_salt_rounds(preferred_cost);
#endif
#ifdef USE_YESCRYPT
	case ENCRYPT_METHOD_YESCRYPT:
		return YESCRYPT_get_salt_cost(preferred_cost);
#endif
	}
	assert(0);
}

static const char *
salt_cost(enum encrypt_method m, unsigned long cost)
{
	switch (m) {
	case ENCRYPT_METHOD_SHA256:
	case ENCRYPT_METHOD_SHA512:
		return SHA_salt_rounds(cost);
#ifdef USE_BCRYPT
	case ENCRYPT_METHOD_BCRYPT:
		return BCRYPT_salt_rounds(cost);
#endif
#ifdef USE_YESCRYPT
	case ENCRYPT_METHOD_YESCRYPT:
		return YESCRYPT_salt_cost(cost);
#endif
	}
	assert(0);
}

static const char *
magnum(enum encrypt_method m)
{
	switch (m) {
	case ENCRYPT_METHOD_SHA256:
		return "$5$";
	case ENCRYPT_METHOD_SHA512:
		return "$6$";
#ifdef USE_BCRYPT
	case ENCRYPT_METHOD_BCRYPT:
		return "$2b$";
#endif
#ifdef USE_YESCRYPT
	case ENCRYPT_METHOD_YESCRYPT:
		return "$y$";
#endif
	}
	assert(0);
}

#if !USE_XCRYPT_GENSALT
static /*@observer@*/const char *
gensalt(enum encrypt_method m)
{
	enum salt_len {
		SALT_LEN_SHA = 16,
#ifdef USE_BCRYPT
		SALT_LEN_BCRYPT = 22,
#endif
#ifdef USE_YESCRYPT
		SALT_LEN_YESCRYPT = 24,
#endif

		SALT_SIZE_MAX  // XXX: Keep the above sorted!
	};

	static char  salt[SALT_SIZE_MAX];

	size_t  len;

	switch (m) {
	case ENCRYPT_METHOD_SHA256:
	case ENCRYPT_METHOD_SHA512:
		len = SALT_LEN_SHA;
		break;
#ifdef USE_BCRYPT
	case ENCRYPT_METHOD_BCRYPT:
		len = SALT_LEN_BCRYPT;
		break;
#endif
#ifdef USE_YESCRYPT
	case ENCRYPT_METHOD_YESCRYPT:
		len = SALT_LEN_YESCRYPT;
		break;
#endif
	}

	p = salt;
	e = &salt[len];
	while (p != NULL)
		p = stpecpy(p, e, l64a(csrand()));

	return salt;
}
#endif /* !USE_XCRYPT_GENSALT */

/*
 * Generate 8 base64 ASCII characters of random salt.
 * Methods can be set with ENCRYPT_METHOD
 *
 * The method can be forced with the 'method' parameter.
 * If NULL, the method will be defined according to the ENCRYPT_METHOD
 * variable, which can be set inside the login.defs file.
 *
 * An additional parameter is provided.
 *  * For the SHA256 and SHA512 method, this specifies the number of rounds
 *    (if not NULL).
 *  * For the YESCRYPT method, this specifies the cost factor (if not NULL).
 */
/*@observer@*/
const char *
crypt_make_salt(/*@null@*//*@observer@*/const char *method, /*@null@*/const long *arg)
{
	static char          result[GENSALT_SETTING_SIZE];
	unsigned long        cost;
	enum encrypt_method  m;

	method = method ?: getdef_str("ENCRYPT_METHOD") ?: "SHA512";

	if (streq(method, "SHA256")) {
		m = ENCRYPT_METHOD_SHA256;
	} else if (streq(method, "SHA512")) {
		m = ENCRYPT_METHOD_SHA512;
#ifdef USE_BCRYPT
	} else if (streq(method, "BCRYPT")) {
		m = ENCRYPT_METHOD_BCRYPT;
#endif /* USE_BCRYPT */
#ifdef USE_YESCRYPT
	} else if (streq(method, "YESCRYPT")) {
		m = ENCRYPT_METHOD_YESCRYPT;
#endif /* USE_YESCRYPT */
	} else {
		fprintf (log_get_logfd(),
			 _("Invalid ENCRYPT_METHOD value: '%s'.\n"
			   "Defaulting to SHA512.\n"),
			 method);
		m = ENCRYPT_METHOD_SHA512;
	}

	strcpy(result, magnum(m));
	cost = get_salt_cost(m, arg);
	assert(strtcat_a(result, salt_cost(m, cost)) != -1);

#if USE_XCRYPT_GENSALT
	char *retval = crypt_gensalt(result, cost, NULL, 0);

	/* Should not happen, but... */
	if (NULL == retval) {
		fprintf (log_get_logfd(),
			 _("Unable to generate a salt from setting "
			   "\"%s\", check your settings in "
			   "ENCRYPT_METHOD and the corresponding "
			   "configuration for your selected hash "
			   "method.\n"), result);

		exit (1);
	}

	return retval;
#else /* USE_XCRYPT_GENSALT */

	assert(strtcat_a(result, gensalt(m)) != -1);

	return result;
#endif /* USE_XCRYPT_GENSALT */
}
