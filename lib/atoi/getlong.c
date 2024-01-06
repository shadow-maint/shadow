// SPDX-FileCopyrightText: 2007-2009, Nicolas François
// SPDX-FileCopyrightText: 2023, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include <config.h>

#include "atoi/getlong.h"


extern inline int getl(const char *numstr, long *result);
extern inline int getul(const char *numstr, unsigned long *result);
