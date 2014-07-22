/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTIL_DEBUG_H
#define _UTIL_DEBUG_H

#include <stdbool.h>
#include <stdio.h>

#define PRINT_ONCE(...) ({ \
                        static bool __printed = 0; \
                        if(!__printed) { \
                            printf(__VA_ARGS__); \
                            __printed=1; \
                        } \
                        })

#endif /* _UTIL_DEBUG_H */
