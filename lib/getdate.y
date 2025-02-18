%{
// SPDX-FileCopyrightText: 2025, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "atoi/a2i/a2s.h"
#include "getdate.h"
#include "string/strchr/strchrcnt.h"
#include "string/strcmp/streq.h"
#include "string/strcmp/strprefix.h"
#include "string/strcmp/strsuffix.h"
#include "string/strspn/stpspn.h"


static int yylex (void);
static int yyerror (const char *s);


#define MAX_BUFF_LEN    128   /* size of buffer to read the date into */

/*
**  An entry in the lexical lookup table.
*/
typedef struct _TABLE {
    const char	*name;
    int		type;
    int		value;
} TABLE;


/*
**  Global variables.  We could get rid of most of these by using a good
**  union as the yacc stack.  (This routine was originally written before
**  yacc had the %union construct.)  Maybe someday; right now we only use
**  the %union very rarely.
*/
static const char	*yyInput;
static long  yyDay;
static long  yyMonth;
static long  yyYear;

%}

%union {
    int			Number;
}

%token	tSNUMBER tUNUMBER

%type	<Number>	tSNUMBER tUNUMBER

%%

spec	: /* NULL */
	;

%%




static int yyerror (MAYBE_UNUSED const char *s)
{
  return 0;
}

static int
yylex (void)
{
  return 0;
}


#define TM_YEAR_ORIGIN 1900


static int parse_date(const char *s);


time_t get_date (const char *p, const time_t *now)
{
  struct tm  tm;

  yyInput = p;

  if (parse_date(p) == -1)
    return -1;

  tm.tm_year = yyYear - TM_YEAR_ORIGIN;
  tm.tm_mon = yyMonth - 1;
  tm.tm_mday = yyDay;
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_isdst = 0;

  return timegm(&tm);
}


static int
parse_date(const char *s)
{
	long  n;

	if (!streq(stpspn(s, "0123456789-"), "")
	 || strchrcnt(s, '-') != 2
	 || strprefix(s, "-")
	 || strsuffix(s, "-")
	 || strstr(s, "--"))
	{
		return -1;
	}

	if (a2sl(&yyYear, s, &s, 10, LONG_MIN, LONG_MAX) == -1 && errno != ENOTSUP)
		return -1;

	if (!strprefix(s++, "-"))
		return -1;

	if (a2sl(&yyMonth, s, &s, 10, LONG_MIN, LONG_MAX) == -1 && errno != ENOTSUP)
		return -1;

	if (!strprefix(s++, "-"))
		return -1;

	if (a2sl(&yyDay, s, NULL, 10, LONG_MIN, LONG_MAX) == -1)
		return -1;

	return 0;
}
