/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_COMPILE_TIME_H
#define _UTILS_COMPILE_TIME_H

#include <autoconf.h>
#include <utils/attribute.h>
#include <utils/stringify.h>

/* Morally, _Static_assert and compile_time_assert are intended to do the same
 * thing. _Static_assert gives more sensible error messages, but is only
 * available in C1X or GCC >= 4.6. Define them both to a common representation
 * that fits our current environment:
 */

#if !defined(CONFIG_LIB_UTILS_NO_STATIC_ASSERT) && (__STDC_VERSION__ >= 201104 || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
    /* Hooray, we're on a modern compiler. */

    #define compile_time_assert(name, expr) _Static_assert((expr), #name)

#else
    /* Boo hiss, no _Static_assert for us. */

    #define compile_time_assert(name, expr) \
        typedef char JOIN(JOIN(JOIN(_assertion_failed_, name), _), __COUNTER__)[(expr) ? 1 : -1] UNUSED

    #define _Static_assert(expr, str) compile_time_assert(_static_assert, (expr))

#endif

#endif /* _UTILS_COMPILE_TIME_H */
