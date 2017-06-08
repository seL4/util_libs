/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

/* macros for compile time attributes */

/* Stub out Clang feature macros for GCC. */
#ifndef __has_attribute
  #define __has_attribute(attrib) 0
#endif
#ifndef __has_extension
  #define __has_extension(ext) 0
#endif
#ifndef __has_feature
  #define __has_feature(feature) 0
#endif

#define ALIAS(sym)   __attribute__((alias(#sym)))
#define ALIGN(n)     __attribute__((__aligned__(n)))
#define ALLOC_SIZE(args...) __attribute__((alloc_size(args)))
#define ASSUME_ALIGNED(args...) __attribute__((assume_aligned(args)))
#define NO_INLINE        __attribute__((noinline))
#define ALWAYS_INLINE __attribute__((always_inline))
#define CLEANUP(fn)  __attribute__((cleanup(fn)))
#if (defined(__clang__) && __has_attribute(cold)) || (!defined(__clang__) && defined(__GNUC__))
  #define COLD       __attribute__((cold))
#else
  #define COLD       /* ignored */
#endif
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#if defined(__clang__) && __has_extension(attribute_unavailable_with_message)
  #define ERROR(msg)   __attribute__((unavailable(msg)))
#elif defined(__GNUC__)
  #define ERROR(msg)   __attribute__((error(msg)))
#else
  /* No good compile-time error feature. Just emit garbage that will force an unclean error. */
  #define ERROR(msg)  __COMPILE_TIME_ERROR_SUPPORT_UNAVAILABLE(msg)
#endif
#if (defined(__clang__) && __has_attribute(hot)) || (!defined(__clang__) && defined(__GNUC__))
  #define HOT        __attribute__((hot))
#else
  #define HOT        /* ignored */
#endif
#define MALLOC       __attribute__((malloc))
#define NONNULL(args...) __attribute__((__nonnull__(args)))
#define NONNULL_ALL  __attribute__((__nonnull__))
#define NORETURN     __attribute__((__noreturn__))
#define PACKED       __attribute__((__packed__))
#define FORMAT(archetype, string_index, first_to_check) \
    __attribute__((format(archetype, string_index, first_to_check)))
#define SECTION(sect) __attribute__((section(sect)))
#define SENTINEL(param) __attribute__((sentinel(param)))
#define SENTINEL_LAST __attribute__((sentinel))
#define UNUSED       __attribute__((__unused__))
#define USED         __attribute__((__used__))
#if defined(__clang__) && !__has_attribute(externally_visible)
  #define VISIBLE /* ignored */
#else
  #define VISIBLE __attribute__((__externally_visible__))
#endif
#define WARNING(msg) __attribute__((warning(msg)))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define WEAK         __attribute__((weak))

#define CONSTRUCTOR_MIN_PRIORITY 101
#define CONSTRUCTOR_MAX_PRIORITY 65535
#define CONSTRUCTOR(priority) __attribute__((constructor(priority)))

/* alloc_align was added to GCC in 4.9 */
#if __has_attribute(alloc_align) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)))
#define ALLOC_ALIGN(arg) __attribute__((alloc_align(arg)))
#else
#define ALLOC_ALIGN(arg) /* ignored */
#endif

/* returns_nonnull was added to GCC in 4.9 */
#if __has_attribute(returns_nonnull) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)))
#define RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define RETURNS_NONNULL /* ignored */
#endif

/* A special case for libsel4 so we can avoid depending on this library.
 * If any other library is caught doing this it will be immolated. */
#ifndef __LIBSEL4_MACROS_H
#define PURE         __attribute__((__pure__))
#define CONST        __attribute__((__const__))
#endif /* __LIBSEL4_MACROS_H */
