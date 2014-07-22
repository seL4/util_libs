/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_ATTRIBUTE_H
#define _UTILS_ATTRIBUTE_H
/* macros for compile time attributes */

#define ALIGN(n)     __attribute__((__aligned__(n)))
#define NO_INLINE        __attribute__((noinline))
#define ALWAYS_INLINE __attribute__((always_inline))
#define DEPRECATED   __attribute__((__deprecated__))
#define ERROR(msg)   __attribute__((error(msg)))
#define NONNULL(args...) __attribute__((__nonnull__(args)))
#define NONNULL_ALL  __attribute__((__nonnull__))
#define NORETURN     __attribute__((__noreturn__))
#define PACKED       __attribute__((__packed__))
#define UNUSED       __attribute__((__unused__))
#define USED         __attribute__((__used__))
#define VISIBLE      __attribute__((__externally_visible__))
#define WARNING(msg) __attribute__((warning(msg)))

/* A special case for libsel4 so we can avoid depending on this library.
 * If any other library is caught doing this it will be immolated. */
#ifndef __LIBSEL4_MACROS_H
#define PURE         __attribute__((__pure__))
#define CONST        __attribute__((__const__))
#endif /* __LIBSEL4_MACROS_H */ 

#endif /* _UTILS_ATTRIBUTE_H */

