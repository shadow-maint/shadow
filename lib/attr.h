#ifndef SHADOW_INCLUDE_LIB_ATTR_H_
#define SHADOW_INCLUDE_LIB_ATTR_H_


#include "config.h"


#if !defined(__has_c_attribute)
# define __has_c_attribute(x)  0
#endif


#if __has_c_attribute(maybe_unused)
# define MAYBE_UNUSED                [[maybe_unused]]
#else
# define MAYBE_UNUSED
#endif

#if __has_c_attribute(noreturn)
# define NORETURN                    [[noreturn]]
#else
# define NORETURN
#endif

#if (__GNUC__ >= 10)
# define format_attr(type, fmt, va)  [[gnu::format(type, fmt, va)]]
# define ATTR_ACCESS(...)            [[gnu::access(__VA_ARGS__)]]
# define ATTR_ALLOC_SIZE(...)        [[gnu::alloc_size(__VA_ARGS__)]]
#else
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

#if __has_c_attribute(gnu::nonstring)
# define ATTR_NONSTRING              [[gnu::nonstring]]
#else
# define ATTR_NONSTRING
#endif


#endif  // include guard
