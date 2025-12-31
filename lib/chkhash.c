#include "config.h"

#include "chkhash.h"

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


/*
 * match_regex - return true if match, false if not
 */
bool 
match_regex(const char *pattern, const char *string) 
{
	regex_t regex;
	int result;

	if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
		return false;

	result = regexec(&regex, string, 0, NULL, 0);
	regfree(&regex);

	return result == 0;
}


/*
 * is_valid_hash - check if the given string is a valid password hash
 *
 * Returns true if the string appears to be a valid hash, false otherwise.
 *
 * regex from: https://man.archlinux.org/man/crypt.5.en
 */
bool 
is_valid_hash(const char *hash) 
{
	// Minimum hash length
	if (strlen(hash) < 27)
		return false;

	// Yescrypt: $y$ + algorithm parameters + $ + salt + $ + 43-char (minimum) hash
	if (match_regex("^\\$y\\$[./A-Za-z0-9]+\\$[./A-Za-z0-9]{1,86}\\$[./A-Za-z0-9]{43}$", hash))
		return true;

	// Bcrypt: $2[abxy]$ + 2-digit cost + $ + 53-char hash
	if (match_regex("^\\$2[abxy]\\$[0-9]{2}\\$[./A-Za-z0-9]{53}$", hash))
		return true;

	// SHA-512: $6$ + salt + $ + 86-char hash
	if (match_regex("^\\$6\\$(rounds=[1-9][0-9]{3,8}\\$)?[^$:\\n]{1,16}\\$[./A-Za-z0-9]{86}$", hash))
		return true;

	// SHA-256: $5$ + salt + $ + 43-char hash
	if (match_regex("^\\$5\\$(rounds=[1-9][0-9]{3,8}\\$)?[^$:\\n]{1,16}\\$[./A-Za-z0-9]{43}$", hash))
		return true;

	// MD5: $1$ + salt + $ + 22-char hash
	if (match_regex("^\\$1\\$[^$:\\n]{1,8}\\$[./A-Za-z0-9]{22}$", hash))
		return true;

	// Not a valid hash
	return false;
}
