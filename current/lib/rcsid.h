/*
 * $Id: rcsid.h,v 1.2 1999/06/07 16:40:44 marekm Exp $
 */
#define PKG_VER " $Package: " PACKAGE " $ $Version: " VERSION " $ "
#if defined(NO_RCSID) || defined(lint)
#define RCSID(x) /* empty */
#else
#if __STDC__
/*
 * This function is never called from anywhere, but it calls itself
 * recursively only to fool gcc to not generate warnings :-).
 */
static const char *rcsid(const char *);
#define RCSID(x) \
  static const char *rcsid(const char *s) { \
  return rcsid(x); }
#else  /* ! __STDC__ */
#define RCSID(x) \
  static char *rcsid(s) char *s; { \
  return rcsid(x); }
#endif /* ! __STDC__ */
#endif
