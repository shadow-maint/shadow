#ifndef SHADOW_INCLUDE_LIB_ATTR_H_
#define SHADOW_INCLUDE_LIB_ATTR_H_


#include "config.h"


#if defined(__GNUC__)
# define unused                      __attribute__((unused))
# define NORETURN                    __attribute__((__noreturn__))
# define format_attr(type, fmt, va)  __attribute__((format(type, fmt, va)))
#else
# define unused
# define NORETURN
# define format_attr(type, fmt, va)
#endif

#if (__GNUC__ >= 11) && !defined(__clang__)
# define ATTR_MALLOC(deallocator)    [[gnu::malloc(deallocator)]]
#else
# define ATTR_MALLOC(deallocator)
#endif


#endif  // include guard
