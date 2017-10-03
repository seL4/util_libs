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

/* Interface for spin locks. Note, the underlying implementation is not
 * seL4-specific. Spin locks should not be used when you have threads of
 * different priorities.
 */

#include <stdbool.h>

typedef int sync_spinlock_t;

static inline int sync_spinlock_init(sync_spinlock_t *lock) {
    *lock = 0;
    return 0;
}

static inline int sync_spinlock_lock(sync_spinlock_t *lock) {
    while (true) {
        int expected = 0;
        if (__atomic_compare_exchange_n(lock, &expected, 1, 1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            break;
        }
    }
    return 0;
}

static inline int sync_spinlock_trylock(sync_spinlock_t *lock) {
    int expected = 0;
    return !__atomic_compare_exchange_n(lock, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static inline int sync_spinlock_unlock(sync_spinlock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
    return 0;
}

static inline int sync_spinlock_destroy(sync_spinlock_t *lock) {
    /* Nothing required. */
    return 0;
}

