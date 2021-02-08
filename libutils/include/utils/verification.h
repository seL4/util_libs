/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* Macros relevant for verification. */

#include <assert.h>

/* This induces a guard containing the given expression in the lifted
 * specification of your function in Isabelle. You can think of this as an
 * assertion that is also visible (and necessary to prove) in Isabelle.
 */
#define GUARD(x) \
    do { \
        assert(x); \
        if (!(x)) { \
            for (;;); \
        } \
    } while (0)
