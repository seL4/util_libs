/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_ASSUME_H
#define _UTILS_ASSUME_H

#include <utils/builtin.h>

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

#endif
