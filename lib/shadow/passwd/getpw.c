// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "shadow/passwd/getpw.h"

#include <pwd.h>


extern inline const struct passwd *getpw_uid_or_nam(const char *u);
