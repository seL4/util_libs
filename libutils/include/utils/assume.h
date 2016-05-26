/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

#include <utils/attribute.h>
#include <utils/builtin.h>
#include <utils/stringify.h>

/* This idiom is a way of passing extra information or hints to GCC. It is only
 * occasionally successful, so don't think of it as a silver optimisation
 * bullet.
 */
#define ASSUME(x) \
    do { \
        if (!(x)) { \
            __builtin_unreachable(); \
        } \
    } while (0)

/* Indicate to the compiler that wherever this macro appears is a cold
 * execution path. That is, it is not performance critical and likely rarely
 * used. A perfect example is error handling code. This gives the compiler a
 * light hint to deprioritise optimisation of this path.
 */
#define COLD_PATH() \
    do { \
        JOIN(cold_path_, __COUNTER__): COLD UNUSED; \
    } while (0)

/* The opposite of `COLD_PATH`. That is, aggressively optimise this path,
 * potentially at the expense of others.
 */
#define HOT_PATH() \
    do { \
        JOIN(hot_path_, __COUNTER__): HOT UNUSED; \
    } while (0)
