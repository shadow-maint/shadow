/*
 * SPDX-FileCopyrightText: 1992 - 1993, Julianne Frances Haugh
 * SPDX-FileCopyrightText: 1996 - 1997, Marek Michałkiewicz
 * SPDX-FileCopyrightText: 2003 - 2005, Tomasz Kłoczko
 * SPDX-FileCopyrightText: 2009       , Nicolas François
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PWAUTH_H
#define _PWAUTH_H

#ifndef USE_PAM
int pw_auth(const char *cipher, const char *user);
#endif				/* !USE_PAM */

#endif /* _PWAUTH_H */
