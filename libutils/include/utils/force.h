/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_FORCE_H
#define _UTILS_FORCE_H
/* macros for forcing the compiler to leave in statments it would
 * normally optimize away */

#include <stdint.h>
#include <utils/attribute.h>
#include <utils/stringify.h>

/* Macro for doing dummy reads
 *
 * Expands to a volatile, unused variable which is set to the value at
 * a given address. It's volatile to prevent the compiler optimizing
 * away a variable that is written but never read, and it's unused to
 * prevent warnings about a variable that's never read.
 */
#define FORCE_READ(address) \
        volatile UNUSED typeof(*address) JOIN(__force__read, __COUNTER__) = *(address)

#endif
