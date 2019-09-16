/*
 * salt.c - generate a random salt string for crypt()
 *
 * Written by Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>,
 * it is in the public domain.
 *
 * l64a was Written by J.T. Conklin <jtc@netbsd.org>. Public domain.
 */

#include <config.h>

#ident "$Id$"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

/* local function prototypes */
static void seedRNG (void);
static /*@observer@*/const char *gensalt (size_t salt_size);
#if defined(USE_SHA_CRYPT) || defined(USE_BCRYPT)
static long shadow_random (long min, long max);
#endif /* USE_SHA_CRYPT || USE_BCRYPT */
#ifdef USE_SHA_CRYPT
static /*@observer@*/const char *SHA_salt_rounds (/*@null@*/int *prefered_rounds);
#endif /* USE_SHA_CRYPT */
#ifdef USE_BCRYPT
static /*@observer@*/const char *gensalt_bcrypt (void);
static /*@observer@*/const char *BCRYPT_salt_rounds (/*@null@*/int *prefered_rounds);
#endif /* USE_BCRYPT */

#ifndef HAVE_L64A
static /*@observer@*/char *l64a(long value)
{
	static char buf[8];
	char *s = buf;
	int digit;
	int i;

	if (value < 0) {
		errno = EINVAL;
		return(NULL);
	}

	for (i = 0; value != 0 && i < 6; i++) {
		digit = value & 0x3f;

		if (digit < 2) {
			*s = digit + '.';
		} else if (digit < 12) {
			*s = digit + '0' - 2;
		} else if (digit < 38) {
			*s = digit + 'A' - 12;
		} else {
			*s = digit + 'a' - 38;
		}

		value >>= 6;
		s++;
	}

	*s = '\0';

	return(buf);
}
#endif /* !HAVE_L64A */

static void seedRNG (void)
{
	struct timeval tv;
	static int seeded = 0;

	if (0 == seeded) {
		(void) gettimeofday (&tv, NULL);
		srandom (tv.tv_sec ^ tv.tv_usec ^ getpid ());
		seeded = 1;
	}
}

/*
 * Add the salt prefix.
 */
#define MAGNUM(array,ch)	(array)[0]=(array)[2]='$',(array)[1]=(ch),(array)[3]='\0'
#ifdef USE_BCRYPT
/* 
 * Using the Prefix $2a$ to enable an anti-collision safety measure in musl libc.
 * Negatively affects a subset of passwords containing the '\xff' character,
 * which is not valid UTF-8 (so "unlikely to cause much annoyance").
 */
#define BCRYPTMAGNUM(array)	(array)[0]=(array)[3]='$',(array)[1]='2',(array)[2]='a',(array)[4]='\0'
#endif /* USE_BCRYPT */

#if defined(USE_SHA_CRYPT) || defined(USE_BCRYPT)
/* It is not clear what is the maximum value of random().
 * We assume 2^31-1.*/
#define RANDOM_MAX 0x7FFFFFFF

/*
 * Return a random number between min and max (both included).
 *
 * It favors slightly the higher numbers.
 */
static long shadow_random (long min, long max)
{
	double drand;
	long ret;
	seedRNG ();
	drand = (double) (max - min + 1) * random () / RANDOM_MAX;
	/* On systems were this is not random() range is lower, we favor
	 * higher numbers of salt. */
	ret = (long) (max + 1 - drand);
	/* And we catch limits, and use the highest number */
	if ((ret < min) || (ret > max)) {
		ret = max;
	}
	return ret;
}
#endif /* USE_SHA_CRYPT || USE_BCRYPT */

#ifdef USE_SHA_CRYPT
/* Default number of rounds if not explicitly specified.  */
#define ROUNDS_DEFAULT 5000
/* Minimum number of rounds.  */
#define ROUNDS_MIN 1000
/* Maximum number of rounds.  */
#define ROUNDS_MAX 999999999
/*
 * Return a salt prefix specifying the rounds number for the SHA crypt methods.
 */
static /*@observer@*/const char *SHA_salt_rounds (/*@null@*/int *prefered_rounds)
{
	static char rounds_prefix[18]; /* Max size: rounds=999999999$ */
	long rounds;

	if (NULL == prefered_rounds) {
		long min_rounds = getdef_long ("SHA_CRYPT_MIN_ROUNDS", -1);
		long max_rounds = getdef_long ("SHA_CRYPT_MAX_ROUNDS", -1);

		if ((-1 == min_rounds) && (-1 == max_rounds)) {
			return "";
		}

		if (-1 == min_rounds) {
			min_rounds = max_rounds;
		}

		if (-1 == max_rounds) {
			max_rounds = min_rounds;
		}

		if (min_rounds > max_rounds) {
			max_rounds = min_rounds;
		}

		rounds = shadow_random (min_rounds, max_rounds);
	} else if (0 == *prefered_rounds) {
		return "";
	} else {
		rounds = *prefered_rounds;
	}

	/* Sanity checks. The libc should also check this, but this
	 * protects against a rounds_prefix overflow. */
	if (rounds < ROUNDS_MIN) {
		rounds = ROUNDS_MIN;
	}

	if (rounds > ROUNDS_MAX) {
		rounds = ROUNDS_MAX;
	}

	(void) snprintf (rounds_prefix, sizeof rounds_prefix,
	                 "rounds=%ld$", rounds);

	return rounds_prefix;
}
#endif /* USE_SHA_CRYPT */

#ifdef USE_BCRYPT
/* Default number of rounds if not explicitly specified.  */
#define B_ROUNDS_DEFAULT 13
/* Minimum number of rounds.  */
#define B_ROUNDS_MIN 4
/* Maximum number of rounds.  */
#define B_ROUNDS_MAX 31
/*
 * Return a salt prefix specifying the rounds number for the BCRYPT method.
 */
static /*@observer@*/const char *BCRYPT_salt_rounds (/*@null@*/int *prefered_rounds)
{
	static char rounds_prefix[4]; /* Max size: 31$ */
	long rounds;

	if (NULL == prefered_rounds) {
		long min_rounds = getdef_long ("BCRYPT_MIN_ROUNDS", -1);
		long max_rounds = getdef_long ("BCRYPT_MAX_ROUNDS", -1);

		if (((-1 == min_rounds) && (-1 == max_rounds)) || (0 == *prefered_rounds)) {
			rounds = B_ROUNDS_DEFAULT;
		}
		else {
			if (-1 == min_rounds) {
				min_rounds = max_rounds;
			}
	
			if (-1 == max_rounds) {
				max_rounds = min_rounds;
			}
	
			if (min_rounds > max_rounds) {
				max_rounds = min_rounds;
			}
	
			rounds = shadow_random (min_rounds, max_rounds);
		}
	} else {
		rounds = *prefered_rounds;
	}

	/* 
	 * Sanity checks. 
	 * Use 19 as an upper bound for now,
	 * because musl doesn't allow rounds >= 20.
	 */
	if (rounds < B_ROUNDS_MIN) {
		rounds = B_ROUNDS_MIN;
	}

	if (rounds > 19) {
		/* rounds = B_ROUNDS_MAX; */
		rounds = 19;
	}

	(void) snprintf (rounds_prefix, sizeof rounds_prefix,
	                 "%2.2ld$", rounds);

	return rounds_prefix;
}

#define BCRYPT_SALT_SIZE 22
/*
 *  Generate a 22 character salt string for bcrypt.
 */
static /*@observer@*/const char *gensalt_bcrypt (void)
{
	static char salt[32];

	salt[0] = '\0';

	seedRNG ();
	strcat (salt, l64a (random()));
	do {
		strcat (salt, l64a (random()));
	} while (strlen (salt) < BCRYPT_SALT_SIZE);

	salt[BCRYPT_SALT_SIZE] = '\0';

	return salt;
}
#endif /* USE_BCRYPT */

/*
 *  Generate salt of size salt_size.
 */
#define MAX_SALT_SIZE 16
#define MIN_SALT_SIZE 8

static /*@observer@*/const char *gensalt (size_t salt_size)
{
	static char salt[32];

	salt[0] = '\0';

	assert (salt_size >= MIN_SALT_SIZE &&
	        salt_size <= MAX_SALT_SIZE);
	seedRNG ();
	strcat (salt, l64a (random()));
	do {
		strcat (salt, l64a (random()));
	} while (strlen (salt) < salt_size);

	salt[salt_size] = '\0';

	return salt;
}

/*
 * Generate 8 base64 ASCII characters of random salt.  If MD5_CRYPT_ENAB
 * in /etc/login.defs is "yes", the salt string will be prefixed by "$1$"
 * (magic) and pw_encrypt() will execute the MD5-based FreeBSD-compatible
 * version of crypt() instead of the standard one.
 * Other methods can be set with ENCRYPT_METHOD
 *
 * The method can be forced with the meth parameter.
 * If NULL, the method will be defined according to the MD5_CRYPT_ENAB and
 * ENCRYPT_METHOD login.defs variables.
 *
 * If meth is specified, an additional parameter can be provided.
 *  * For the SHA256 and SHA512 method, this specifies the number of rounds
 *    (if not NULL).
 */
/*@observer@*/const char *crypt_make_salt (/*@null@*//*@observer@*/const char *meth, /*@null@*/void *arg)
{
	/* Max result size for the SHA methods:
	 *  +3		$5$
	 *  +17		rounds=999999999$
	 *  +16		salt
	 *  +1		\0
	 */
	static char result[40];
	size_t salt_len = 8;
	const char *method;

	result[0] = '\0';

	if (NULL != meth)
		method = meth;
	else {
		method = getdef_str ("ENCRYPT_METHOD");
		if (NULL == method) {
			method = getdef_bool ("MD5_CRYPT_ENAB") ? "MD5" : "DES";
		}
	}

	if (0 == strcmp (method, "MD5")) {
		MAGNUM(result, '1');
#ifdef USE_BCRYPT
	} else if (0 == strcmp (method, "BCRYPT")) {
		BCRYPTMAGNUM(result);
		strcat(result, BCRYPT_salt_rounds((int *)arg));
#endif /* USE_BCRYPT */
#ifdef USE_SHA_CRYPT
	} else if (0 == strcmp (method, "SHA256")) {
		MAGNUM(result, '5');
		strcat(result, SHA_salt_rounds((int *)arg));
		salt_len = (size_t) shadow_random (8, 16);
	} else if (0 == strcmp (method, "SHA512")) {
		MAGNUM(result, '6');
		strcat(result, SHA_salt_rounds((int *)arg));
		salt_len = (size_t) shadow_random (8, 16);
#endif /* USE_SHA_CRYPT */
	} else if (0 != strcmp (method, "DES")) {
		fprintf (stderr,
			 _("Invalid ENCRYPT_METHOD value: '%s'.\n"
			   "Defaulting to DES.\n"),
			 method);
		result[0] = '\0';
	}

	/*
	 * Concatenate a pseudo random salt.
	 */
	assert (sizeof (result) > strlen (result) + salt_len);
#ifdef USE_BCRYPT
	if (0 == strcmp (method, "BCRYPT")) {
		strncat (result, gensalt_bcrypt (),
				 sizeof (result) - strlen (result) - 1);
		return result;
	} else {
#endif /* USE_BCRYPT */	
		strncat (result, gensalt (salt_len),
			 sizeof (result) - strlen (result) - 1);
#ifdef USE_BCRYPT
	}
#endif /* USE_BCRYPT */	

	return result;
}

