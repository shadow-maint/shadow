// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _SHADOW_INCLUDE_LIB_FIELDS_H_
#define _SHADOW_INCLUDE_LIB_FIELDS_H_


#include "config.h"

#include <stddef.h>


#define valid_field(field, illegal)  valid_field_(field, "" illegal "")
#define change_field_a(buf, prompt)  change_field(buf, countof(buf), prompt)


int valid_field_(const char *field, const char *illegal);
void change_field(char *buf, size_t n, const char *prompt);


#endif  // include guard
