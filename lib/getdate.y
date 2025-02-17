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
#include "string/strcmp/streq.h"
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
**  Meridian:  am, pm, or 24-hour style.
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;


/*
**  Global variables.  We could get rid of most of these by using a good
**  union as the yacc stack.  (This routine was originally written before
**  yacc had the %union construct.)  Maybe someday; right now we only use
**  the %union very rarely.
*/
static const char	*yyInput;
static int	yyDayOrdinal;
static int	yyDayNumber;
static int	yyHaveDate;
static int	yyHaveDay;
static int	yyHaveRel;
static int	yyHaveTime;
static int	yyDay;
static int	yyHour;
static int	yyMinutes;
static int	yyMonth;
static int	yySeconds;
static int	yyYear;
static MERIDIAN	yyMeridian;
static int	yyRelDay;
static int	yyRelHour;
static int	yyRelMinutes;
static int	yyRelMonth;
static int	yyRelSeconds;
static int	yyRelYear;

%}

%union {
    int			Number;
    enum _MERIDIAN	Meridian;
}

%token	tAGO tDAY tDAY_UNIT tHOUR_UNIT tID
%token	tMERIDIAN tMINUTE_UNIT tMONTH tMONTH_UNIT
%token	tSEC_UNIT tSNUMBER tUNUMBER tYEAR_UNIT

%type	<Number>	tDAY tDAY_UNIT tHOUR_UNIT tMINUTE_UNIT
%type	<Number>	tMONTH tMONTH_UNIT
%type	<Number>	tSEC_UNIT tSNUMBER tUNUMBER tYEAR_UNIT
%type	<Meridian>	tMERIDIAN o_merid

%%

spec	: /* NULL */
	| spec item
	;

item	: time {
	    yyHaveTime++;
	}
	| date {
	    yyHaveDate++;
	}
	| day {
	    yyHaveDay++;
	}
	| rel {
	    yyHaveRel++;
	}
	| number
	;

time	: tUNUMBER tMERIDIAN {
	    yyHour = $1;
	    yyMinutes = 0;
	    yySeconds = 0;
	    yyMeridian = $2;
	}
	| tUNUMBER ':' tUNUMBER o_merid {
	    yyHour = $1;
	    yyMinutes = $3;
	    yySeconds = 0;
	    yyMeridian = $4;
	}
	| tUNUMBER ':' tUNUMBER {
	    yyHour = $1;
	    yyMinutes = $3;
	    yyMeridian = MER24;
	}
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid {
	    yyHour = $1;
	    yyMinutes = $3;
	    yySeconds = $5;
	    yyMeridian = $6;
	}
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER {
	    yyHour = $1;
	    yyMinutes = $3;
	    yySeconds = $5;
	    yyMeridian = MER24;
	}
	;

day	: tDAY {
	    yyDayOrdinal = 1;
	    yyDayNumber = $1;
	}
	| tDAY ',' {
	    yyDayOrdinal = 1;
	    yyDayNumber = $1;
	}
	| tUNUMBER tDAY {
	    yyDayOrdinal = $1;
	    yyDayNumber = $2;
	}
	;

date	: tUNUMBER '/' tUNUMBER {
	    yyMonth = $1;
	    yyDay = $3;
	}
	| tUNUMBER '/' tUNUMBER '/' tUNUMBER {
	  /* Interpret as YYYY/MM/DD if $1 >= 1000, otherwise as MM/DD/YY.
	     The goal in recognizing YYYY/MM/DD is solely to support legacy
	     machine-generated dates like those in an RCS log listing.  If
	     you want portability, use the ISO 8601 format.  */
	  if ($1 >= 1000)
	    {
	      yyYear = $1;
	      yyMonth = $3;
	      yyDay = $5;
	    }
	  else
	    {
	      yyMonth = $1;
	      yyDay = $3;
	      yyYear = $5;
	    }
	}
	| tUNUMBER tSNUMBER tSNUMBER {
	    /* ISO 8601 format.  yyyy-mm-dd.  */
	    yyYear = $1;
	    yyMonth = -$2;
	    yyDay = -$3;
	}
	| tUNUMBER tMONTH tSNUMBER {
	    /* e.g. 17-JUN-1992.  */
	    yyDay = $1;
	    yyMonth = $2;
	    yyYear = -$3;
	}
	| tMONTH tUNUMBER {
	    yyMonth = $1;
	    yyDay = $2;
	}
	| tMONTH tUNUMBER ',' tUNUMBER {
	    yyMonth = $1;
	    yyDay = $2;
	    yyYear = $4;
	}
	| tUNUMBER tMONTH {
	    yyMonth = $2;
	    yyDay = $1;
	}
	| tUNUMBER tMONTH tUNUMBER {
	    yyMonth = $2;
	    yyDay = $1;
	    yyYear = $3;
	}
	;

rel	: relunit tAGO {
	    yyRelSeconds = -yyRelSeconds;
	    yyRelMinutes = -yyRelMinutes;
	    yyRelHour = -yyRelHour;
	    yyRelDay = -yyRelDay;
	    yyRelMonth = -yyRelMonth;
	    yyRelYear = -yyRelYear;
	}
	| relunit
	;

relunit	: tUNUMBER tYEAR_UNIT {
	    yyRelYear += $1 * $2;
	}
	| tSNUMBER tYEAR_UNIT {
	    yyRelYear += $1 * $2;
	}
	| tYEAR_UNIT {
	    yyRelYear += $1;
	}
	| tUNUMBER tMONTH_UNIT {
	    yyRelMonth += $1 * $2;
	}
	| tSNUMBER tMONTH_UNIT {
	    yyRelMonth += $1 * $2;
	}
	| tMONTH_UNIT {
	    yyRelMonth += $1;
	}
	| tUNUMBER tDAY_UNIT {
	    yyRelDay += $1 * $2;
	}
	| tSNUMBER tDAY_UNIT {
	    yyRelDay += $1 * $2;
	}
	| tDAY_UNIT {
	    yyRelDay += $1;
	}
	| tUNUMBER tHOUR_UNIT {
	    yyRelHour += $1 * $2;
	}
	| tSNUMBER tHOUR_UNIT {
	    yyRelHour += $1 * $2;
	}
	| tHOUR_UNIT {
	    yyRelHour += $1;
	}
	| tUNUMBER tMINUTE_UNIT {
	    yyRelMinutes += $1 * $2;
	}
	| tSNUMBER tMINUTE_UNIT {
	    yyRelMinutes += $1 * $2;
	}
	| tMINUTE_UNIT {
	    yyRelMinutes += $1;
	}
	| tUNUMBER tSEC_UNIT {
	    yyRelSeconds += $1 * $2;
	}
	| tSNUMBER tSEC_UNIT {
	    yyRelSeconds += $1 * $2;
	}
	| tSEC_UNIT {
	    yyRelSeconds += $1;
	}
	;

number	: tUNUMBER
          {
	    if ((yyHaveTime != 0) && (yyHaveDate != 0) && (yyHaveRel == 0))
	      yyYear = $1;
	    else
	      {
		if ($1>10000)
		  {
		    yyHaveDate++;
		    yyDay= ($1)%100;
		    yyMonth= ($1/100)%100;
		    yyYear = $1/10000;
		  }
		else
		  {
		    yyHaveTime++;
		    if ($1 < 100)
		      {
			yyHour = $1;
			yyMinutes = 0;
		      }
		    else
		      {
		    	yyHour = $1 / 100;
		    	yyMinutes = $1 % 100;
		      }
		    yySeconds = 0;
		    yyMeridian = MER24;
		  }
	      }
	  }
	;

o_merid	: /* NULL */
	  {
	    $$ = MER24;
	  }
	| tMERIDIAN
	  {
	    $$ = $1;
	  }
	;

%%

/* Month and day table. */
static TABLE const MonthDayTable[] = {
    { "january",	tMONTH,  1 },
    { "february",	tMONTH,  2 },
    { "march",		tMONTH,  3 },
    { "april",		tMONTH,  4 },
    { "may",		tMONTH,  5 },
    { "june",		tMONTH,  6 },
    { "july",		tMONTH,  7 },
    { "august",		tMONTH,  8 },
    { "september",	tMONTH,  9 },
    { "sept",		tMONTH,  9 },
    { "october",	tMONTH, 10 },
    { "november",	tMONTH, 11 },
    { "december",	tMONTH, 12 },
    { "sunday",		tDAY, 0 },
    { "monday",		tDAY, 1 },
    { "tuesday",	tDAY, 2 },
    { "tues",		tDAY, 2 },
    { "wednesday",	tDAY, 3 },
    { "wednes",		tDAY, 3 },
    { "thursday",	tDAY, 4 },
    { "thur",		tDAY, 4 },
    { "thurs",		tDAY, 4 },
    { "friday",		tDAY, 5 },
    { "saturday",	tDAY, 6 },
    { NULL, 0, 0 }
};

/* Time units table. */
static TABLE const UnitsTable[] = {
    { "year",		tYEAR_UNIT,	1 },
    { "month",		tMONTH_UNIT,	1 },
    { "fortnight",	tDAY_UNIT,	14 },
    { "week",		tDAY_UNIT,	7 },
    { "day",		tDAY_UNIT,	1 },
    { "hour",		tHOUR_UNIT,	1 },
    { "minute",		tMINUTE_UNIT,	1 },
    { "min",		tMINUTE_UNIT,	1 },
    { "second",		tSEC_UNIT,	1 },
    { "sec",		tSEC_UNIT,	1 },
    { NULL, 0, 0 }
};

/* Assorted relative-time words. */
static TABLE const OtherTable[] = {
    { "tomorrow",	tMINUTE_UNIT,	1 * 24 * 60 },
    { "yesterday",	tMINUTE_UNIT,	-1 * 24 * 60 },
    { "today",		tMINUTE_UNIT,	0 },
    { "now",		tMINUTE_UNIT,	0 },
    { "last",		tUNUMBER,	-1 },
    { "this",		tMINUTE_UNIT,	0 },
    { "next",		tUNUMBER,	2 },
    { "first",		tUNUMBER,	1 },
/*  { "second",		tUNUMBER,	2 }, */
    { "third",		tUNUMBER,	3 },
    { "fourth",		tUNUMBER,	4 },
    { "fifth",		tUNUMBER,	5 },
    { "sixth",		tUNUMBER,	6 },
    { "seventh",	tUNUMBER,	7 },
    { "eighth",		tUNUMBER,	8 },
    { "ninth",		tUNUMBER,	9 },
    { "tenth",		tUNUMBER,	10 },
    { "eleventh",	tUNUMBER,	11 },
    { "twelfth",	tUNUMBER,	12 },
    { "ago",		tAGO,	1 },
    { NULL, 0, 0 }
};




static int yyerror (MAYBE_UNUSED const char *s)
{
  return 0;
}

static int ToHour (int Hours, MERIDIAN Meridian)
{
  switch (Meridian)
    {
    case MER24:
      if (Hours < 0 || Hours > 23)
	return -1;
      return Hours;
    case MERam:
      if (Hours < 1 || Hours > 12)
	return -1;
      if (Hours == 12)
	Hours = 0;
      return Hours;
    case MERpm:
      if (Hours < 1 || Hours > 12)
	return -1;
      if (Hours == 12)
	Hours = 0;
      return Hours + 12;
    default:
      abort ();
    }
  /* NOTREACHED */
}

static int ToYear (int Year)
{
  if (Year < 0)
    Year = -Year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if (Year < 69)
    Year += 2000;
  else if (Year < 100)
    Year += 1900;

  return Year;
}

static int LookupWord (char *buff)
{
  register char *p;
  register const TABLE *tp;
  int i;
  bool abbrev;

  /* Make it lowercase. */
  for (p = buff; !streq(p, ""); p++)
    if (isupper (*p))
      *p = tolower (*p);

  if (streq(buff, "am") || streq(buff, "a.m."))
    {
      yylval.Meridian = MERam;
      return tMERIDIAN;
    }
  if (streq(buff, "pm") || streq(buff, "p.m."))
    {
      yylval.Meridian = MERpm;
      return tMERIDIAN;
    }

  /* See if we have an abbreviation for a month. */
  if (strlen (buff) == 3)
    abbrev = true;
  else if (strlen (buff) == 4 && buff[3] == '.')
    {
      abbrev = true;
      stpcpy(&buff[3], "");
    }
  else
    abbrev = false;

  for (tp = MonthDayTable; tp->name; tp++)
    {
      if (abbrev)
	{
	  if (strncmp (buff, tp->name, 3) == 0)
	    {
	      yylval.Number = tp->value;
	      return tp->type;
	    }
	}
      else if (streq(buff, tp->name))
	{
	  yylval.Number = tp->value;
	  return tp->type;
	}
    }

  for (tp = UnitsTable; tp->name; tp++)
    if (streq(buff, tp->name))
      {
	yylval.Number = tp->value;
	return tp->type;
      }

  /* Strip off any plural and try the units table again. */
  i = strlen (buff) - 1;
  if (buff[i] == 's')
    {
      stpcpy(&buff[i], "");
      for (tp = UnitsTable; tp->name; tp++)
	if (streq(buff, tp->name))
	  {
	    yylval.Number = tp->value;
	    return tp->type;
	  }
      buff[i] = 's';		/* Put back for "this" in OtherTable. */
    }

  for (tp = OtherTable; tp->name; tp++)
    if (streq(buff, tp->name))
      {
	yylval.Number = tp->value;
	return tp->type;
      }

  return tID;
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
      if (isalpha (c))
	{
	  for (p = buff; (c = *yyInput++, isalpha (c)) || c == '.';)
	    if (p < &buff[sizeof buff - 1])
	      *p++ = c;
          stpcpy(p, "");
	  yyInput--;
	  return LookupWord (buff);
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
  struct tm  tm, *tmp;
  time_t Start;

  yyInput = p;
  Start = now ? *now : time(NULL);
  tmp = gmtime(&Start);
  yyYear = tmp->tm_year + TM_YEAR_ORIGIN;
  yyMonth = tmp->tm_mon + 1;
  yyDay = tmp->tm_mday;
  yyHour = tmp->tm_hour;
  yyMinutes = tmp->tm_min;
  yySeconds = tmp->tm_sec;
  yyMeridian = MER24;
  yyRelSeconds = 0;
  yyRelMinutes = 0;
  yyRelHour = 0;
  yyRelDay = 0;
  yyRelMonth = 0;
  yyRelYear = 0;
  yyHaveDate = 0;
  yyHaveDay = 0;
  yyHaveRel = 0;
  yyHaveTime = 0;

  if (yyparse ()
      || yyHaveTime > 1 || yyHaveDate > 1 || yyHaveDay > 1)
    return -1;

  tm.tm_year = ToYear (yyYear) - TM_YEAR_ORIGIN + yyRelYear;
  tm.tm_mon = yyMonth - 1 + yyRelMonth;
  tm.tm_mday = yyDay + yyRelDay;
  if ((yyHaveTime != 0) ||
      ( (yyHaveRel != 0) && (yyHaveDate == 0) && (yyHaveDay == 0) ))
    {
      tm.tm_hour = ToHour (yyHour, yyMeridian);
      if (tm.tm_hour < 0)
	return -1;
      tm.tm_min = yyMinutes;
      tm.tm_sec = yySeconds;
    }
  else
    {
      tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    }
  tm.tm_hour += yyRelHour;
  tm.tm_min += yyRelMinutes;
  tm.tm_sec += yyRelSeconds;
  tm.tm_isdst = 0;

  Start = timegm(&tm);

  if (Start == (time_t) -1)
    {
	return Start;
    }

  if (yyHaveDay && !yyHaveDate)
    {
      tm.tm_mday += ((yyDayNumber - tm.tm_wday + 7) % 7
		     + 7 * (yyDayOrdinal - (0 < yyDayOrdinal)));
      Start = timegm(&tm);
      if (Start == (time_t) -1)
	return Start;
    }

  return Start;
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
