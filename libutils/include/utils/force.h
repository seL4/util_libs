/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
