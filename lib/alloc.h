#ifndef SHADOW_INCLUDE_LIB_MALLOC_H_
#define SHADOW_INCLUDE_LIB_MALLOC_H_


#include <stddef.h>

/* xmalloc.c */
extern /*@maynotreturn@*/ /*@only@*//*@out@*//*@notnull@*/void *xmalloc (size_t size)
  /*@ensures MaxSet(result) == (size - 1); @*/;
extern /*@maynotreturn@*/ /*@only@*//*@notnull@*/char *xstrdup (const char *);


#endif  // include guard
