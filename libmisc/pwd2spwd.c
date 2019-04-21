/*
 * Copyright (c) 1989 - 1994, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2003 - 2005, Tomasz Kłoczko
 * Copyright (c) 2007 - 2009, Nicolas François
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the copyright holders or contributors may not be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#ident "$Id$"

#ifndef USE_PAM

#include <sys/types.h>
#include "prototypes.h"
#include "defines.h"
#include <pwd.h>

/*
 * pwd_to_spwd - create entries for new spwd structure
 *
 *	pwd_to_spwd() creates a new (struct spwd) containing the
 *	information in the pointed-to (struct passwd).
 */

struct spwd *pwd_to_spwd (const struct passwd *pw)
{
	static struct spwd sp;

	/*
	 * Nice, easy parts first.  The name and passwd map directly
	 * from the old password structure to the new one.
	 */
	sp.sp_namp = pw->pw_name;
	sp.sp_pwdp = pw->pw_passwd;

	{
		/*
		 * Defaults used if there is no pw_age information.
		 */
		sp.sp_min = 0;
		sp.sp_max = (10000L * DAY) / SCALE;
		sp.sp_lstchg = (long) gettime () / SCALE;
		if (0 == sp.sp_lstchg) {
			/* Better disable aging than requiring a password
			 * change */
			sp.sp_lstchg = -1;
		}
	}

	/*
	 * These fields have no corresponding information in the password
	 * file.  They are set to uninitialized values.
	 */
	sp.sp_warn = -1;
	sp.sp_expire = -1;
	sp.sp_inact = -1;
	sp.sp_flag = SHADOW_SP_FLAG_UNSET;

	return &sp;
}
#else				/* USE_PAM */
extern int errno;	/* warning: ANSI C forbids an empty source file */
#endif				/* !USE_PAM */

