/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <assert.h>
#include <limits.h>
#include <stddef.h>

/** \brief Atomically increment an integer, accounting for possible overflow.
 *
 * @param x Pointer to integer to increment.
 * @param[out] oldval Previous value of the integer. May be written to even if
 *   the increment fails.
 * @param success_memorder The memory order to enforce
 * @return 0 if the increment succeeds, non-zero if it would cause an overflow.
 */
static inline int sync_atomic_increment_safe(volatile int *x, int *oldval, int success_memorder) {
    assert(x != NULL);
    assert(oldval != NULL);
    do {
        *oldval = *x;
        if (*oldval == INT_MAX) {
            /* We would overflow */
            return -1;
        }
    } while (!__atomic_compare_exchange_n(x, oldval, *oldval + 1, 1, success_memorder, __ATOMIC_RELAXED));
    return 0;
}

/** \brief Atomically decrement an integer, accounting for possible overflow.
 *
 * @param x Pointer to integer to decrement.
 * @param[out] oldval Previous value of the integer. May be written to even if
 *   the decrement fails.
 * @param success_memorder The memory order to enforce if the decrement is successful
 * @return 0 if the decrement succeeds, non-zero if it would cause an overflow.
 */
static inline int sync_atomic_decrement_safe(volatile int *x, int *oldval, int success_memorder) {
    assert(x != NULL);
    assert(oldval != NULL);
    do {
        *oldval = *x;
        if (*oldval == INT_MIN) {
            /* We would overflow */
            return -1;
        }
    } while (!__atomic_compare_exchange_n(x, oldval, *oldval - 1, 1, success_memorder, __ATOMIC_RELAXED));
    return 0;
}

/* Atomically increment an integer and return its new value. */
static inline int sync_atomic_increment(volatile int *x, int memorder) {
    return __atomic_add_fetch(x, 1, memorder);
}

/* Atomically decrement an integer and return its new value. */
static inline int sync_atomic_decrement(volatile int *x, int memorder) {
    return __atomic_sub_fetch(x, 1, memorder);
}

