/*
 * SPDX-FileCopyrightText: 2022, Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */

#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>

#ident "$Id$"

#include "defines.h"
#include "prototypes.h"


#if !defined(INET_ADDRSTRLENMAX)
#define INET_ADDRSTRLENMAX  MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)
#endif


/*
 * SYNOPSIS
 *	const char *inet_sockaddr2str(const struct sockaddr *sa);
 *
 * DESCRIPTION
 *	This function is similar to inet_ntop(3).  It transforms an address
 *	in 'struct in_addr' or 'struct in6_addr' form into a human-readable
 *	string.
 *
 *	It receives a sockaddr structure, which is simpler to pass after
 *	receiving it from getaddrinfo(3).  However, this function is not
 *	reentrant, and like inet_ntoa(3), it uses an internal buffer, for
 *	simplicity; anyway we're not in a multithreaded program, and it
 *	doesn't contain any sensitive data, so it's fine to use static
 *	storage here.
 *
 * RETURN VALUE
 *	This function returns a pointer to a statically allocated buffer,
 *	which subsequent calls will overwrite.
 *
 *	On error, it returns NULL.
 *
 * ERRORS
 *	EAFNOSUPPORT
 *		The address family in sa->sa_family is not AF_INET or AF_INET6.
 *
 * CAVEATS
 *	This function is not reentrant.
 */


const char *
inet_sockaddr2str(const struct sockaddr *sa)
{
	struct sockaddr_in   *sin;
	struct sockaddr_in6  *sin6;

	static char          buf[INET_ADDRSTRLENMAX];

	switch (sa->sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *) sa;
		inet_ntop(AF_INET, &sin->sin_addr, buf, NITEMS(buf));
		return buf;
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *) sa;
		inet_ntop(AF_INET6, &sin6->sin6_addr, buf, NITEMS(buf));
		return buf;
	default:
		errno = EAFNOSUPPORT;
		return NULL;
	}
}
