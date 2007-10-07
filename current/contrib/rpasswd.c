/* rpasswd.c -- restricted `passwd' wrapper.
   Copyright (C) 1996 Adam Solesby, Joshua Cowan

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This program is meant to be a wrapper for use with `sudo' and your
   system's `passwd' program.  It is *probably* secure, but there is no
   warranty (see above).  If you find errors or security holes, please
   email me; please include a complete description of the problem in
   your message in addition to any patches.  */

/* This program currently assumes that the arguments given on the
   command line are user names to pass to the `passwd' program; it loops
   through the arguments calling `passwd' on each one.  It might be
   better to pass all remaining arguments after `--' to `passwd' (to
   e.g., change the user's shell instead of the password by giving it
   the `-s' option).  */

/* Written by Adam Solesby <adam@shack.com>.  */
/* Rewritten by Joshua Cowan <jcowan@hermit.reslife.okstate.edu>.  */

/* Usage: rpasswd USERNAME...
   Enforce password-changing guidelines.

     --check[=file]   check configuration information; if FILE is given,
                        use that instead of the standard configuration
                        file `./rpasswd.conf'
     --help           display this help and exit
     --version        output version information and exit

   You may never change a superuser's password with this command.
   Changing certain other users' passwords may also be forbidden; for
   details of who's passwords may not be changed, try `rpasswd --check'.  */

/* TODO:

   - Make this more portable.  It currently depends on several
   GNU/Linux-specific features.  */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <pwd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>

/* This is the absolute path to the `passwd' program on your system.  */
#define _PATH_PASSWD  "/usr/bin/passwd"

/* This is the absolute path to the configuration file.  */
#define _PATH_RPASSWD_CONF  "/etc/rpasswd.conf"

/* Don't change the password of any user with a uid equal to or below
   this number--no matter what the configuration file says.  */
#define UID_PWD_CHANGE_FLOOR  100

/* Everything past this point should probably be left alone.  */

/* These are the facility and priority (respectively) used by the syslog
   functions.  */
#define LOG_FACILITY  LOG_AUTH
#define LOG_PRIORITY  LOG_WARNING

/* The name this program was run with.  */
char *program_name;

/* The version information for this program.  */
char *version_string = "1.2";

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output then exit.  */
static int show_version;

/* If nonzero, check the configuration file for errors and print the
   list of restrictions on the standard output, then exit.  */
static int check_only;

struct user_list
{
  char *name;
  struct user_list *next;
};

struct config_info
{
  /* Don't change the password for any user with a uid less than or
     equal to this number.  */
  uid_t minimum_uid;

  /* Don't change the password for any user matching this list of user
     names.  */
  struct user_list *inviolate_user_names;
};

static const struct option long_options[] =
{
  {"check", optional_argument, 0, 10},
  {"version", no_argument, &show_version, 1},
  {"help", no_argument, &show_help, 1},
  {0, 0, 0, 0}
};

static struct config_info *get_config_info ();
static int dump_config_info ();
static void *xmalloc ();
static void *xrealloc ();
static void xsyslog (int, const char *, ...);
static void dal_error (int, int, const char *, ...);

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s USERNAME...\n", program_name);
      fputs ("\
Enforce password-changing guidelines.\n\
\n\
  --check[=file]   check configuration information; if FILE is given,\n\
                     use that instead of the standard configuration file\n\
                     `"_PATH_RPASSWD_CONF"'\n\
  --help           display this help and exit\n\
  --version        output version information and exit\n",
	     stdout);

      printf ("\n\
You may never change a superuser's password with this command.  Changing\n\
certain other users' passwords may also be forbidden; for details of\n\
who's passwords may not be changed, try `%s --check'.\n",
	      program_name);
    }

  exit (status);
}

int
main (argc, argv)
     int argc;
     char **argv;
{
  char *executing_user_name;
  char *config_file_name = _PATH_RPASSWD_CONF;
  int opt;
  struct config_info *config;

  /* Setting values of global variables.  */
  program_name = argv[0];

  while ((opt = getopt_long (argc, argv, "", long_options, 0))
	 != EOF)
    switch (opt)
      {
      case 0:
	break;

      case 10:
	check_only = 1;
	if (optarg)
	  config_file_name = optarg;
	break;

      default:
	usage (1);
      }

  if (show_version)
    {
      printf ("rpasswd %s\n", version_string);
      return 0;
    }

  if (show_help)
    {
      usage (0);
    }

  if (check_only)
    {
      dump_config_info (config_file_name);
      exit (0);
    }

  if (optind >= argc)
    {
      fprintf (stderr, "%s: missing argument\n", program_name);
      usage (1);
    }

  /* FIXME: does `sudo' set the real user id to the effective user id?
     If so, this won't work as intended: We want to get the name of the
     user who ran `sudo'.  I am reluctant to use `getlogin' for obvious
     reasons, but it may be better than nothing.  Maybe someone who
     actually has `sudo' installed can tell me if this works, or how to
     fix it if it doesn't.  --JC */
  do
    {
      struct passwd *pwd;
      uid_t uid = getuid ();
      
      pwd = getpwuid (uid);

      if (!pwd || !pwd->pw_name)
	{
	  xsyslog (LOG_PRIORITY,
		   "Unknown user (uid #%d) attempted to change password for `%s'.",
		   uid, argv[optind]);
	  fprintf (stderr, "%s: you do not exist, go away\n",
		   program_name);
	  exit (1);
	}
      else
	executing_user_name = pwd->pw_name;
    }
  while (0);

  config = get_config_info (config_file_name);

  for (; optind < argc; optind++)
    {
      int immutable_p = 0;
      struct user_list *user_names = config->inviolate_user_names;

      /* Make sure we weren't given an illegal user name.  */
      for (; user_names; user_names = user_names->next)
	{
	  if (strcmp (argv[optind], user_names->name)
	      == 0)
	    {
	      immutable_p = 1;
	      break;
	    }
	}

      if (!immutable_p)
	{
	  struct passwd *pwd;

	  pwd = getpwnam (argv[optind]);

	  if (!pwd)
	    {
	      fprintf (stderr, "%s: invalid user `%s'\n",
		       program_name, argv[optind]);

	      continue;
	    }
	  else if (pwd->pw_uid <= config->minimum_uid)
	    immutable_p = 1;
	}

      if (immutable_p)
	{
	  xsyslog (LOG_PRIORITY,
		   "`%s' attempted to change password for `%s'.",
		   executing_user_name, argv[optind]);
	  fprintf (stderr,
		   "You are not allowed to change the password for `%s'.\n",
		   argv[optind]);
	}
      else
	{
	  int pid, status;

	  pid = fork ();
	  switch (pid)
	    {
	    case -1:
	      dal_error (1, errno, "cannot fork");

	    case 0:
	      execl (_PATH_PASSWD, _PATH_PASSWD, "--", argv[optind], 0);
	      _exit (1);

	    default:
	      while (wait (&status) != pid)
		;

	      if (status & 0xFFFF)
		dal_error (1, EIO, "%s", _PATH_PASSWD);

	      break;
	    }
	}
    }

  exit (0);
}

/* Get configuration information from FILE and return a pointer to a
   `config_info' structure containing that information.  It currently
   does minimal checking of the validity of the information.

   This function never returns NULL, even when the configuration file is
   empty.  If the configuration file doesn't exist, it just exits with a
   failed exit status.  */

static struct config_info *
get_config_info (file)
     const char *const file;
{
  FILE *config_file;
  struct config_info *config;
  char linebuf[BUFSIZ];
  unsigned int lineno = 0;

  config = (struct config_info *) xmalloc (sizeof (struct config_info));
  config->minimum_uid = (uid_t) 0;
  config->inviolate_user_names = 0;

  config_file = fopen (file, "r");
  if (!config_file)
    dal_error (1, errno, "%s", file);

  if (fseek (config_file, 0L, SEEK_SET))
    dal_error (1, errno, "%s", file);

  while (fgets (linebuf, BUFSIZ, config_file))
    {
      int len, i, uid_found = 0;

      lineno++;

      len = strlen (linebuf);

      /* Chomp any whitespace off the end of the line.  */
      while (isspace (linebuf[len - 1]))
	linebuf[--len] = '\0';

      /* If this line is empty or a comment, skip it and go to the next.  */
      if (len == 0 || *linebuf == '#')
	continue;

      for (i = 0; i < len; i++)
	if (!isalnum (linebuf[i])
	    && linebuf[i] != '.'
	    && linebuf[i] != '-'
	    && linebuf[i] != '_')
	  {
	    dal_error (1, 0, "%s:%u: invalid user name `%s'",
		       file, lineno, linebuf);
	  }

      /* Only accept positive integers as candidates for `minimum_uid'.  */
      for (i = 0; i < len; i++)
	if (!isdigit (linebuf[i]))
	  break;

      if (!uid_found && i == len)
	{
	  unsigned long num;

	  errno = 0;
	  num = strtoul (linebuf, 0, 10);
	  config->minimum_uid = (uid_t) num;

	  if (errno || config->minimum_uid != num)
	    dal_error (1, 0, "%s:%u: `%s' out of range",
		       file, lineno, linebuf);

	  uid_found = 1;
	}
      else
	{
	  struct user_list *tail = config->inviolate_user_names;
	  struct user_list *user_names = 0;

	  /* This could be more efficient, but makes the list of users
             printed out with the `--check' switch easier to read.  */

	  for (; tail; tail = tail->next)
	    {
	      if (strcmp (linebuf, tail->name) == 0)
		break;

	      user_names = tail;
	    }

	  if (!tail)
	    {
	      tail = user_names;

	      user_names = xmalloc (sizeof (struct user_list));
	      user_names->name = strcpy (xmalloc (len + 1), linebuf);
	      user_names->next = 0;

	      if (!config->inviolate_user_names)
		config->inviolate_user_names = user_names;
	      else
		tail->next = user_names;
	    }
	}
    }

  fclose (config_file);

  if (config->minimum_uid < UID_PWD_CHANGE_FLOOR)
    config->minimum_uid = UID_PWD_CHANGE_FLOOR;

  return config;
}

/* Dump the configuration info contained in FILE to the standard output.  */

static int
dump_config_info (file)
     char *file;
{
  struct config_info *config;

  config = get_config_info (file);

  printf ("\
The lowest uid who's password may be changed is number %d.  Changing
the following users' passwords is also forbidden:\n",
	  config->minimum_uid + 1);

  if (!config->inviolate_user_names)
    {
      printf ("\n  (no users listed in configuration file `%s')\n",
	      file);
    }
  else
    {
      int column;
      struct user_list *user_names = config->inviolate_user_names;

      for (column = 73; user_names; user_names = user_names->next)
	{
	  int name_len = strlen (user_names->name);

	  if (user_names->next)
	    name_len++;

	  column += name_len;

	  if (column > 72)
	    {
	      fputs ("\n  ", stdout);
	      column = name_len + 2;
	    }
	  else if (column - name_len > 0)
	    {
	      fputc (' ', stdout);
	      column++;
	    }

	  fputs (user_names->name, stdout);

	  if (user_names->next)
	    fputc (',', stdout);
	}

      fputc ('\n', stdout);
    }

  return 0;
}

static void *
xmalloc (n)
     size_t n;
{
  void *ptr;

  ptr = malloc (n);

  if (!ptr)
    {
      fprintf (stderr, "%s: Memory exhausted\n", program_name);
      exit (1);
    }

  return ptr;
}

static void *
xrealloc (ptr, n)
     void *ptr;
     size_t n;
{
  ptr = realloc (ptr, n);

  if (!ptr)
    {
      fprintf (stderr, "%s: Memory exhausted\n", program_name);
      exit (1);
    }

  return ptr;
}

static void
xsyslog (int priority, const char *format, ...)
{
  va_list args;
  static int logfd_opened = 0;

  if (!logfd_opened)
    {
      openlog (program_name, LOG_PID, LOG_FACILITY);
      logfd_opened = 1;
    }

  va_start (args, format);
  vsyslog (priority, format, args);
  va_end (args);
}

/* Format and display MESSAGE on the standard error and send it to the
   system logger.  If ERRNUM is not 0, append the system error message
   corresponding to ERRNUM to the output.  If STATUS is not 0, exit with
   an exit status of STATUS.  */

static void
dal_error (int status, int errnum, const char *message, ...)
{
  va_list args;
  size_t bufsize;
  char *formatted_message;

  fflush (stdout);

  bufsize = strlen (message) * 2;
  formatted_message = (char *) xmalloc (bufsize);

  va_start (args, message);

  while (1)
    {
      int printed;
      printed = vsnprintf (formatted_message, bufsize, message, args);

      if ((size_t) printed < bufsize)
	break;

      bufsize *= 2;
      formatted_message	= xrealloc (formatted_message, bufsize);
    }

  va_end (args);

  if (errnum)
    {
      char *error_message = strerror (errnum);

      formatted_message
	= xrealloc (formatted_message,
		    (strlen (formatted_message)
		     + strlen (error_message)
		     + 3));

      strcat (formatted_message, ": ");
      strcat (formatted_message, error_message);
    }

  fprintf (stderr, "%s: %s\n", program_name, formatted_message);

  xsyslog (LOG_PRIORITY, "%s", formatted_message);

  free (formatted_message);
  fflush (stderr);

  if (status)
    {
      closelog ();
      exit (status);
    }
}
