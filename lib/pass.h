// SPDX-FileCopyrightText: 2022-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_PASS_H_
#define SHADOW_INCLUDE_LIB_PASS_H_


#include "config.h"

#include <limits.h>
#include <readpassphrase.h>
#include <stddef.h>
#include <stdio.h>

#include "alloc/malloc.h"
#include "attr.h"


// There is also a limit in PAM (PAM_MAX_RESP_SIZE), currently set to 512.
#ifndef  PASS_MAX
# define PASS_MAX  (BUFSIZ - 1)
#endif


// Similar to getpass(3), but free of its problems, and using alloca(3).
#define getpassa(prompt)       getpass2(passalloca(), prompt)
#define getpassa_stdin()       getpass2_stdin(passalloca())

// Similar to getpass(3), but free of its problems, and get the buffer in $1.
#define getpass2(buf, prompt)  readpass(buf, prompt, RPP_REQUIRE_TTY)
#define getpass2_stdin(buf)    readpass(buf, "", RPP_STDIN)

#define passalloca()           alloca_T(1, pass_t)


typedef typeof(char [PASS_MAX + 2])  pass_t;


pass_t *passzero(pass_t *pass);

ATTR_MALLOC(passzero)
pass_t *readpass(pass_t *pass, const char *restrict prompt, int flags);


#endif  // include guard
