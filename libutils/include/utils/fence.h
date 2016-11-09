/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#pragma once

/* The fence is designed to try and prevent the compiler optimizing across code boundaries
   that we don't want it to. The reason for preventing optimization is so that things like
   overhead calculations aren't unduly influenced */
#define COMPILER_MEMORY_FENCE() asm volatile("" ::: "memory")
