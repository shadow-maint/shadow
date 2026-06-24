// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "pass.h"

#include <errno.h>
#include <readpassphrase.h>
#include <stddef.h>
#include <string.h>

#include "sizeof.h"
#include "string/memset/memzero.h"


#define READPASSPHRASE(prompt, buf, flags)                            \
(                                                                     \
	readpassphrase(prompt, buf, countof(buf), flags)              \
)


pass_t *
passzero(pass_t *pass)
{
	if (pass == NULL)
		return NULL;

	return memzero_a(*pass);
}


// readpassphrase(3), but detect truncation, and memzero() on error.
pass_t *
readpass(pass_t *pass, const char *restrict prompt, int flags)
{
	size_t  len;

	/*
	 * Since we want to support passwords upto PASS_MAX, we need
	 * PASS_MAX bytes for the password itself, and one more byte for
	 * the terminating '\0'.  We also want to detect truncation, and
	 * readpassphrase(3) doesn't detect it, so we need some trick.
	 * Let's add one more byte, and if the password uses it, it
	 * means the introduced password was longer than PASS_MAX.
	 */
	if (READPASSPHRASE(prompt, *pass, flags) == NULL)
		goto fail;

	len = strlen(*pass);
	if (len == countof(*pass) - 1) {
		errno = ENOBUFS;
		goto fail;
	}

	return pass;
fail:
	memzero_a(*pass);
	return NULL;
}
