// SPDX-FileCopyrightText: 2024-2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#include "config.h"

#include "search/cmp/cmp.h"


extern inline int CMP__int(const void *key, const void *elt);
extern inline int CMP__long(const void *key, const void *elt);
extern inline int CMP__u_int(const void *key, const void *elt);
extern inline int CMP__u_long(const void *key, const void *elt);
