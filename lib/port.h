/*
 * Copyright (c) 1989 - 1991, Julianne Frances Haugh
 * Copyright (c) 1996 - 1997, Marek Michałkiewicz
 * Copyright (c) 2005       , Tomasz Kłoczko
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

/*
 * port.h - structure of /etc/porttime
 *
 *	$Id$
 *
 *	Each entry in /etc/porttime consists of a TTY device
 *	name or "*" to indicate all TTY devices, followed by
 *	a list of 1 or more user IDs or "*" to indicate all
 *	user names, followed by a list of zero or more valid
 *	login times.  Login time entries consist of zero or
 *	more day names (Su, Mo, Tu, We, Th, Fr, Sa, Wk, Al)
 *	followed by a pair of time values in HHMM format
 *	separated by a "-".
 */

/*
 * PORTS - Name of system port access time file.
 * PORT_IDS - Allowable number of IDs per entry.
 * PORT_TTY - Allowable number of TTYs per entry.
 * PORT_TIMES - Allowable number of time entries per entry.
 * PORT_DAY - Day of the week to a bit value (0 = Sunday).
 */

#define	PORTS	"/etc/porttime"
#define	PORT_IDS	64
#define	PORT_TTY	64
#define	PORT_TIMES	24
#define	PORT_DAY(day)	(1<<(day))

/*
 *	pt_names - pointer to array of device names in /dev/
 *	pt_users - pointer to array of applicable user IDs.
 *	pt_times - pointer to list of allowable time periods.
 */

struct port {
	char **pt_names;
	char **pt_users;
	struct pt_time *pt_times;
};

/*
 *	t_days - bit array for each day of the week (0 = Sunday)
 *	t_start - starting time for this entry
 *	t_end - ending time for this entry
 */

struct pt_time {
	short t_days;
	short t_start;
	short t_end;
};
