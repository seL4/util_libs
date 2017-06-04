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

/* macros for forcing the compiler to leave in statments it would
 * normally optimize away */

#include <utils/attribute.h>
#include <utils/stringify.h>

/* Macro for doing dummy reads
 *
 * Forces a memory read access to the given address.
 */
#define FORCE_READ(address) \
    do { \
        typeof(*(address)) *_ptr = (address); \
        asm volatile ("" : "=m"(*_ptr) : "r"(*_ptr)); \
    } while (0)
