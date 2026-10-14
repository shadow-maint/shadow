#include "config.h"

#include "chkhash.h"

#include <locale.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"


/*
 * match_regex - return true if match, false if not
 */
bool 
match_regex(const char *pattern, const char *string) 
{
	regex_t regex;
	int result;
	locale_t l, c;

	c = newlocale(LC_ALL_MASK, "C", (locale_t) 0);
	if (c == (locale_t) 0) {
		return false;
	}
	l = uselocale(c);
	if (l == (locale_t) 0) {
		freelocale(c);
		return false;
	}

	result = REG_NOMATCH;
	if (regcomp(&regex, pattern, REG_EXTENDED) == 0) {
		result = regexec(&regex, string, 0, NULL, 0);
		regfree(&regex);
	}

	uselocale(l);
	freelocale(c);
	return result == 0;
}


/*
 * is_valid_hash - check if the string is a valid shadow(5) 2nd field.
 *
 * regex from: https://man.archlinux.org/man/crypt.5.en
 */
bool 
is_valid_hash(const char *hash) 
{
	// Password temporarily locked
	hash = strprefix(hash, "!") ?: hash;

	// Passwordless account; discouraged
	if (streq(hash, ""))
		return true;

	// Password permanently locked (and forgotten)
	if (streq(hash, "*"))
		return true;

	// Minimum hash length
	if (strlen(hash) < 48)
		return false;

	// Yescrypt: $y$ + algorithm parameters + $ + salt + $ + 43-char hash
	if (match_regex("^\\$y\\$[./A-Za-z0-9]+\\$[./A-Za-z0-9]{1,86}\\$[./A-Za-z0-9]{43}$", hash))
		return true;

	// Bcrypt: $2[abxy]$ + 2-digit cost + $ + 53-char hash
	if (match_regex("^\\$2[abxy]\\$[0-9]{2}\\$[./A-Za-z0-9]{53}$", hash))
		return true;

	// SHA-512: $6$ + salt + $ + 86-char hash
	if (match_regex("^\\$6\\$(rounds=[1-9][0-9]{3,8}\\$)?[^$:\n]{1,16}\\$[./A-Za-z0-9]{86}$", hash))
		return true;

	// SHA-256: $5$ + salt + $ + 43-char hash
	if (match_regex("^\\$5\\$(rounds=[1-9][0-9]{3,8}\\$)?[^$:\n]{1,16}\\$[./A-Za-z0-9]{43}$", hash))
		return true;

	// Not a valid hash
	return false;
}
