/* Copyright (c) 1999 SuSE GmbH Nuerenberg, Germany
   Author: Thorsten Kukuk <kukuk@suse.de> */

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

/* Version number of the daemon interface */
#define NSCD_VERSION 2
/* Path for the Unix domain socket.  */
#define _PATH_NSCDSOCKET "/var/run/.nscd_socket"

/* Available services.  */
typedef enum
{
  GETPWBYNAME,
  GETPWBYUID,
  GETGRBYNAME,
  GETGRBYGID,
  GETHOSTBYNAME,
  GETHOSTBYNAMEv6,
  GETHOSTBYADDR,
  GETHOSTBYADDRv6,
  LASTDBREQ = GETHOSTBYADDRv6,
  SHUTDOWN,             /* Shut the server down.  */
  GETSTAT,              /* Get the server statistic.  */
  INVALIDATE,           /* Invalidate one special cache.  */
  LASTREQ
} request_type;

/* Header common to all requests */
typedef struct
{
  int version;          /* Version number of the daemon interface.  */
  request_type type;    /* Service requested.  */
#if defined(__alpha__)
  int64_t key_len;      /* Key length is 64bit on Alpha.  */
#else
  int32_t key_len;      /* Key length, 32bit on most plattforms.  */
#endif
} request_header;

/* Create a socket connected to a name.  */
static int
nscd_open_socket (void)
{
  struct sockaddr_un addr;
  int sock;

  sock = socket (PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;

  addr.sun_family = AF_UNIX;
  assert (sizeof (addr.sun_path) >= sizeof (_PATH_NSCDSOCKET));
  strcpy (addr.sun_path, _PATH_NSCDSOCKET);
  if (connect (sock, (struct sockaddr *) &addr, sizeof (addr)) < 0)
    {
      close (sock);
      return -1;
    }

  return sock;
}

int
nscd_flush_cache (char *service)
{
  int sock = nscd_open_socket ();
  request_header req;
  ssize_t nbytes;

  if (sock == -1)
    return -1;

  req.version = NSCD_VERSION;
  req.type = INVALIDATE;
  req.key_len = strlen (service) + 1;
  nbytes = write (sock, &req, sizeof (request_header));
  if (nbytes != sizeof (request_header))
    {
      close (sock);
      return -1;
    }

  nbytes = write (sock, (void *)service, req.key_len);

  close (sock);
  return (nbytes != req.key_len ? (-1) : 0);
}
