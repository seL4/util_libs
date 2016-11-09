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

/* The compiler won't re-order memory accesses across this fence. */
#define COMPILER_MEMORY_FENCE() __atomic_signal_fence(__ATOMIC_ACQ_REL)

/* The compiler won't re-order memory writes to after this fence. */
#define COMPILER_MEMORY_RELEASE() __atomic_signal_fence(__ATOMIC_RELEASE)

/* The compiler won't re-order memory reads to before this fence. */
#define COMPILER_MEMORY_ACQUIRE() __atomic_signal_fence(__ATOMIC_ACQUIRE)
