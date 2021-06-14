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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "prototypes.h"
#include "defines.h"
#include "getdef.h"

/* Add the salt prefix. */
#define MAGNUM(array,ch)	(array)[0]=(array)[2]='$',(array)[1]=(ch),(array)[3]='\0'

#ifdef USE_BCRYPT
/* Use $2b$ as prefix for compatibility with OpenBSD's bcrypt. */
#define BCRYPTMAGNUM(array)	(array)[0]=(array)[3]='$',(array)[1]='2',(array)[2]='b',(array)[4]='\0'
#define BCRYPT_SALT_SIZE 22
/* Default number of rounds if not explicitly specified.  */
#define B_ROUNDS_DEFAULT 13
/* Minimum number of rounds.  */
#define B_ROUNDS_MIN 4
/* Maximum number of rounds.  */
#define B_ROUNDS_MAX 31
#endif /* USE_BCRYPT */

#ifdef USE_SHA_CRYPT
/* Fixed salt len for sha{256,512}crypt. */
#define SHA_CRYPT_SALT_SIZE 16
/* Default number of rounds if not explicitly specified.  */
#define SHA_ROUNDS_DEFAULT 5000
/* Minimum number of rounds.  */
#define SHA_ROUNDS_MIN 1000
/* Maximum number of rounds.  */
#define SHA_ROUNDS_MAX 999999999
#endif

#ifdef USE_YESCRYPT
/*
 * Default number of base64 characters used for the salt.
 * 24 characters gives a 144 bits (18 bytes) salt. Unlike the more
 * traditional 128 bits (16 bytes) salt, this 144 bits salt is always
 * represented by the same number of base64 characters without padding
 * issue, even with a non-standard base64 encoding scheme.
 */
#define YESCRYPT_SALT_SIZE 24
/* Default cost if not explicitly specified.  */
#define Y_COST_DEFAULT 5
/* Minimum cost.  */
#define Y_COST_MIN 1
/* Maximum cost.  */
#define Y_COST_MAX 11
#endif

/* Fixed salt len for md5crypt. */
#define MD5_CRYPT_SALT_SIZE 8

/* Generate salt of size salt_size. */
#define MAX_SALT_SIZE 44
#define MIN_SALT_SIZE 8

/* Maximum size of the generated salt string. */
#define GENSALT_SETTING_SIZE 100

/* local function prototypes */
static void seedRNG (void);
static /*@observer@*/const char *gensalt (size_t salt_size);
#if defined(USE_SHA_CRYPT) || defined(USE_BCRYPT)
static long shadow_random (long min, long max);
#endif /* USE_SHA_CRYPT || USE_BCRYPT */
#ifdef USE_SHA_CRYPT
static /*@observer@*/void SHA_salt_rounds_to_buf (char *buf, /*@null@*/int *prefered_rounds);
#endif /* USE_SHA_CRYPT */
#ifdef USE_BCRYPT
static /*@observer@*/void BCRYPT_salt_rounds_to_buf (char *buf, /*@null@*/int *prefered_rounds);
#endif /* USE_BCRYPT */
#ifdef USE_YESCRYPT
static /*@observer@*/void YESCRYPT_salt_cost_to_buf (char *buf, /*@null@*/int *prefered_cost);
#endif /* USE_YESCRYPT */

#ifndef HAVE_L64A
static /*@observer@*/char *l64a (long value)
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

	return buf;
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
/*
 * Fill a salt prefix specifying the rounds number for the SHA crypt methods
 * to a buffer.
 */
static /*@observer@*/void SHA_salt_rounds_to_buf (char *buf, /*@null@*/int *prefered_rounds)
{
	unsigned long rounds;
	const size_t buf_begin = strlen (buf);

	if (NULL == prefered_rounds) {
		long min_rounds = getdef_long ("SHA_CRYPT_MIN_ROUNDS", -1);
		long max_rounds = getdef_long ("SHA_CRYPT_MAX_ROUNDS", -1);

		if ((-1 == min_rounds) && (-1 == max_rounds)) {
			rounds = SHA_ROUNDS_DEFAULT;
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

		rounds = (unsigned long) shadow_random (min_rounds, max_rounds);
	} else if (0 == *prefered_rounds) {
		rounds = SHA_ROUNDS_DEFAULT;
	} else {
		rounds = (unsigned long) *prefered_rounds;
	}

	/* Sanity checks. The libc should also check this, but this
	 * protects against a rounds_prefix overflow. */
	if (rounds < SHA_ROUNDS_MIN) {
		rounds = SHA_ROUNDS_MIN;
	}

	if (rounds > SHA_ROUNDS_MAX) {
		rounds = SHA_ROUNDS_MAX;
	}

	/* Nothing to do here if SHA_ROUNDS_DEFAULT is used. */
	if (rounds == SHA_ROUNDS_DEFAULT) {
		return;
	}

	/* Check if the result buffer is long enough. */
	assert (GENSALT_SETTING_SIZE > buf_begin + 17);

	(void) snprintf (buf + buf_begin, 18, "rounds=%lu$", rounds);
}
#endif /* USE_SHA_CRYPT */

#ifdef USE_BCRYPT
/*
 * Fill a salt prefix specifying the rounds number for the BCRYPT method
 * to a buffer.
 */
static /*@observer@*/void BCRYPT_salt_rounds_to_buf (char *buf, /*@null@*/int *prefered_rounds)
{
	unsigned long rounds;
	const size_t buf_begin = strlen (buf);

	if (NULL == prefered_rounds) {
		long min_rounds = getdef_long ("BCRYPT_MIN_ROUNDS", -1);
		long max_rounds = getdef_long ("BCRYPT_MAX_ROUNDS", -1);

		if ((-1 == min_rounds) && (-1 == max_rounds)) {
			rounds = B_ROUNDS_DEFAULT;
		} else {
			if (-1 == min_rounds) {
				min_rounds = max_rounds;
			}

			if (-1 == max_rounds) {
				max_rounds = min_rounds;
			}

			if (min_rounds > max_rounds) {
				max_rounds = min_rounds;
			}

			rounds = (unsigned long) shadow_random (min_rounds, max_rounds);
		}
	} else if (0 == *prefered_rounds) {
		rounds = B_ROUNDS_DEFAULT;
	} else {
		rounds = (unsigned long) *prefered_rounds;
	}

	/* Sanity checks. */
	if (rounds < B_ROUNDS_MIN) {
		rounds = B_ROUNDS_MIN;
	}

	/*
	 * Use 19 as an upper bound for now,
	 * because musl doesn't allow rounds >= 20.
	 */
	if (rounds > 19) {
		/* rounds = B_ROUNDS_MAX; */
		rounds = 19;
	}

	/* Check if the result buffer is long enough. */
	assert (GENSALT_SETTING_SIZE > buf_begin + 3);

	(void) snprintf (buf + buf_begin, 4, "%2.2lu$", rounds);
}
#endif /* USE_BCRYPT */

#ifdef USE_YESCRYPT
/*
 * Fill a salt prefix specifying the cost for the YESCRYPT method
 * to a buffer.
 */
static /*@observer@*/void YESCRYPT_salt_cost_to_buf (char *buf, /*@null@*/int *prefered_cost)
{
	unsigned long cost;
	const size_t buf_begin = strlen (buf);

	if (NULL == prefered_cost) {
		cost = getdef_num ("YESCRYPT_COST_FACTOR", Y_COST_DEFAULT);
	} else if (0 == *prefered_cost) {
		cost = Y_COST_DEFAULT;
	} else {
		cost = (unsigned long) *prefered_cost;
	}

	/* Sanity checks. */
	if (cost < Y_COST_MIN) {
		cost = Y_COST_MIN;
	}

	if (cost > Y_COST_MAX) {
		cost = Y_COST_MAX;
	}

	/* Check if the result buffer is long enough. */
	assert (GENSALT_SETTING_SIZE > buf_begin + 3);

	buf[buf_begin + 0] = 'j';
	if (cost < 3) {
		buf[buf_begin + 1] = 0x36 + cost;
	} else if (cost < 6) {
		buf[buf_begin + 1] = 0x34 + cost;
	} else {
		buf[buf_begin + 1] = 0x3b + cost;
	}
	buf[buf_begin + 2] = cost >= 3 ? 'T' : '5';
	buf[buf_begin + 3] = '$';
	buf[buf_begin + 4] = '\0';
}
#endif /* USE_YESCRYPT */

static /*@observer@*/const char *gensalt (size_t salt_size)
{
	static char salt[MAX_SALT_SIZE + 6];

	memset (salt, '\0', MAX_SALT_SIZE + 6);

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
 * If NULL, the method will be defined according to the ENCRYPT_METHOD
 * variable, and if not set according to the MD5_CRYPT_ENAB variable,
 * which can both be set inside the login.defs file.
 *
 * If meth is specified, an additional parameter can be provided.
 *  * For the SHA256 and SHA512 method, this specifies the number of rounds
 *    (if not NULL).
 *  * For the YESCRYPT method, this specifies the cost factor (if not NULL).
 */
/*@observer@*/const char *crypt_make_salt (/*@null@*//*@observer@*/const char *meth, /*@null@*/void *arg)
{
	static char result[GENSALT_SETTING_SIZE];
	size_t salt_len = MAX_SALT_SIZE;
	const char *method;

	memset (result, '\0', GENSALT_SETTING_SIZE);

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
		salt_len = MD5_CRYPT_SALT_SIZE;
#ifdef USE_BCRYPT
	} else if (0 == strcmp (method, "BCRYPT")) {
		BCRYPTMAGNUM(result);
		salt_len = BCRYPT_SALT_SIZE;
		BCRYPT_salt_rounds_to_buf (result, (int *) arg);
#endif /* USE_BCRYPT */
#ifdef USE_YESCRYPT
	} else if (0 == strcmp (method, "YESCRYPT")) {
		MAGNUM(result, 'y');
		salt_len = YESCRYPT_SALT_SIZE;
		YESCRYPT_salt_cost_to_buf (result, (int *) arg);
#endif /* USE_YESCRYPT */
#ifdef USE_SHA_CRYPT
	} else if (0 == strcmp (method, "SHA256")) {
		MAGNUM(result, '5');
		salt_len = SHA_CRYPT_SALT_SIZE;
		SHA_salt_rounds_to_buf (result, (int *) arg);
	} else if (0 == strcmp (method, "SHA512")) {
		MAGNUM(result, '6');
		salt_len = SHA_CRYPT_SALT_SIZE;
		SHA_salt_rounds_to_buf (result, (int *) arg);
#endif /* USE_SHA_CRYPT */
	} else if (0 != strcmp (method, "DES")) {
		fprintf (shadow_logfd,
			 _("Invalid ENCRYPT_METHOD value: '%s'.\n"
			   "Defaulting to DES.\n"),
			 method);
		salt_len = MAX_SALT_SIZE;
		memset (result, '\0', GENSALT_SETTING_SIZE);
	}

	/* Check if the result buffer is long enough. */
	assert (GENSALT_SETTING_SIZE > strlen (result) + salt_len);

	/* Concatenate a pseudo random salt. */
	strncat (result, gensalt (salt_len),
		 GENSALT_SETTING_SIZE - strlen (result) - 1);

	return result;
}
