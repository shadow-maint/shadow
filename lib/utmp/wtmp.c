// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "utmp/wtmp.h"

#include <utmpx.h>


#if !defined(HAVE_UPDWTMPX)
extern inline void updwtmpx(const char *filename, const struct utmpx *ut);
#endif
