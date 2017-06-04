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
