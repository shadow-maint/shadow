/*
 * SPDX-FileCopyrightText: 2019-2023, Alejandro Colomar <alx@kernel.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SHADOW_INCLUDE_LIBMISC_MUST_BE_H_
#define SHADOW_INCLUDE_LIBMISC_MUST_BE_H_


#include <config.h>

#include <assert.h>


/*
 * SYNOPSIS
 *	int must_be(bool e);
 *
 * ARGUMENTS
 *	e	Expression to be asserted.
 *
 * DESCRIPTION
 *	This macro fails compilation if 'e' is false.  If 'e' is true,
 *	it returns (int) 0, so it doesn't affect the expression in which
 *	it is contained.
 *
 *	This macro is similar to static_assert(3).  While
 *	static_assert(3) can only be used where a statement is allowed,
 *	this must_be() macro can be used wherever an expression is
 *	allowed.
 *
 * RETURN VALUE
 *	0
 *
 * ERRORS
 *	If 'e' is false, the compilation will fail, as when using
 *	static_assert(3).
 *
 * EXAMPLES
 *	#define must_be_array(a)  must_be(is_array(a))
 *
 *	#define NITEMS(a)  (sizeof(a) / sizeof(*(a)) + must_be_array(a))
 *
 *	int foo[42];
 *	int bar[NITEMS(foo)];
 */


#define must_be(e)                                                            \
(                                                                             \
	0 * (int) sizeof(                                                     \
		struct {                                                      \
			static_assert(e, "");                                 \
			int ISO_C_forbids_a_struct_with_no_members_;          \
		}                                                             \
	)                                                                     \
)


#endif  // include guard
