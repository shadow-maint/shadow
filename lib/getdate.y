%{
/*
**  Originally written by Steven M. Bellovin <smb@research.att.com> while
**  at the University of North Carolina at Chapel Hill.  Later tweaked by
**  a couple of people on Usenet.  Completely overhauled by Rich $alz
**  <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990;
**
**  This grammar has 13 shift/reduce conflicts.
**
**  This code is in the public domain and has no copyright.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Since the code of getdate.y is not included in the Emacs executable
   itself, there is no need to #define static in this file.  Even if
   the code were included in the Emacs executable, it probably
   wouldn't do any harm to #undef it here; this will only cause
   problems if we try to write to a static variable, which I don't
   think this code needs to do.  */
#ifdef emacs
# undef static
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "attr.h"
#include "getdate.h"
#include "string/strspn/stpspn.h"


/* Some old versions of bison generate parsers that use bcopy.
   That loses on systems that don't provide the function, so we have
   to redefine it here.  */
#if !defined (HAVE_BCOPY) && !defined (bcopy)
# define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

#define yymaxdepth gd_maxdepth
#define yyparse gd_parse
#define yylex   gd_lex
#define yyerror gd_error
#define yylval  gd_lval
#define yychar  gd_char
#define yydebug gd_debug
#define yypact  gd_pact
#define yyr1    gd_r1
#define yyr2    gd_r2
#define yydef   gd_def
#define yychk   gd_chk
#define yypgo   gd_pgo
#define yyact   gd_act
#define yyexca  gd_exca
#define yyerrflag gd_errflag
#define yynerrs gd_nerrs
#define yyps    gd_ps
#define yypv    gd_pv
#define yys     gd_s
#define yy_yys  gd_yys
#define yystate gd_state
#define yytmp   gd_tmp
#define yyv     gd_v
#define yy_yyv  gd_yyv
#define yyval   gd_val
#define yylloc  gd_lloc
#define yyreds  gd_reds          /* With YYDEBUG defined */
#define yytoks  gd_toks          /* With YYDEBUG defined */
#define yylhs   gd_yylhs
#define yylen   gd_yylen
#define yydefred gd_yydefred
#define yydgoto gd_yydgoto
#define yysindex gd_yysindex
#define yyrindex gd_yyrindex
#define yygindex gd_yygindex
#define yytable  gd_yytable
#define yycheck  gd_yycheck

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
static int	yyDay;
static int	yyMonth;
static int	yyYear;

%}

%union {
    int			Number;
}

%token	tSNUMBER tUNUMBER

%type	<Number>	tSNUMBER tUNUMBER

%%

spec	: /* NULL */
	| spec item
	;

item	: date
	| number
	;

date	: tUNUMBER tSNUMBER tSNUMBER {
	    /* ISO 8601 format.  yyyy-mm-dd.  */
	    yyYear = $1;
	    yyMonth = -$2;
	    yyDay = -$3;
	}
	;

number	: tUNUMBER
          {
		    yyDay= ($1)%100;
		    yyMonth= ($1/100)%100;
		    yyYear = $1/10000;
	  }
	;

%%




static int yyerror (MAYBE_UNUSED const char *s)
{
  return 0;
}

static int
yylex (void)
{
  register char c;
  register char *p;
  char buff[20];
  int Count;
  int sign;

  for (;;)
    {
      yyInput = stpspn(yyInput, " \t");

      if (isdigit (c = *yyInput) || c == '-' || c == '+')
	{
	  if (c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      if (!isdigit (*++yyInput))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  for (yylval.Number = 0; isdigit (c = *yyInput++);)
	    yylval.Number = 10 * yylval.Number + c - '0';
	  yyInput--;
	  if (sign < 0)
	    yylval.Number = -yylval.Number;
	  return (0 != sign) ? tSNUMBER : tUNUMBER;
	}
      if (c != '(')
	return *yyInput++;
      Count = 0;
      do
	{
	  c = *yyInput++;
	  if (c == '\0')
	    return c;
	  if (c == '(')
	    Count++;
	  else if (c == ')')
	    Count--;
	}
      while (Count > 0);
    }
}

#define TM_YEAR_ORIGIN 1900

time_t get_date (const char *p, const time_t *now)
{
  struct tm  tm;

  yyInput = p;

  if (yyparse())
    return -1;

  tm.tm_year = yyYear - TM_YEAR_ORIGIN;
  tm.tm_mon = yyMonth - 1;
  tm.tm_mday = yyDay;
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_isdst = 0;

  return timegm(&tm);
}

#if	defined (TEST)

int
main(void)
{
  char buff[MAX_BUFF_LEN + 1];
  time_t d;

  (void) printf ("Enter date, or blank line to exit.\n\t> ");
  (void) fflush (stdout);

  buff[MAX_BUFF_LEN] = 0;
  while (fgets (buff, MAX_BUFF_LEN, stdin) && buff[0])
    {
      d = get_date(buff, NULL);
      if (d == -1)
	(void) printf ("Bad format - couldn't convert.\n");
      else
	(void) printf ("%s", ctime (&d));
      (void) printf ("\t> ");
      (void) fflush (stdout);
    }
  exit (0);
  /* NOTREACHED */
}
#endif /* defined (TEST) */
