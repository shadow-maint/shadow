// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_LIST_H_
#define SHADOW_INCLUDE_LIB_LIST_H_


#include "config.h"

#include <stdbool.h>
#include <stddef.h>


extern /*@only@*/char **add_list (/*@returned@*/ /*@only@*/char **, const char *);
extern /*@only@*/char **del_list (/*@returned@*/ /*@only@*/char **, const char *);
extern /*@only@*/char **dup_list (char *const *);
extern void free_list (char **);
extern bool is_on_list (char *const *list, const char *member);
extern /*@only@*/char **comma_to_list (const char *);
extern char **acsv2ls(char *s);
extern int csv2ls(char *s, size_t n, char *ls[restrict n]);


#endif  // include guard
