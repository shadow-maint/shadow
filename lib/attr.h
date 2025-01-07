#ifndef SHADOW_INCLUDE_LIB_ATTR_H_
#define SHADOW_INCLUDE_LIB_ATTR_H_


#include "config.h"


#if (__GNUC__ >= 10)
# define MAYBE_UNUSED                [[gnu::unused]]
# define NORETURN                    [[gnu::__noreturn__]]
# define format_attr(type, fmt, va)  [[gnu::format(type, fmt, va)]]
# define ATTR_ACCESS(...)            [[gnu::access(__VA_ARGS__)]]
# define ATTR_ALLOC_SIZE(...)        [[gnu::alloc_size(__VA_ARGS__)]]
#else
# define MAYBE_UNUSED
# define NORETURN
# define format_attr(type, fmt, va)
# define ATTR_ACCESS(...)
# define ATTR_ALLOC_SIZE(...)
#endif

#if (__GNUC__ >= 11) && !defined(__clang__)
# define ATTR_MALLOC(deallocator)    [[gnu::malloc(deallocator)]]
#else
# define ATTR_MALLOC(deallocator)
#endif

#if (__GNUC__ >= 14)
# define ATTR_STRING(i)              [[gnu::null_terminated_string_arg(i)]]
#else
# define ATTR_STRING(i)
#endif


#endif  // include guard
