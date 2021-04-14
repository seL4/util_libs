/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <errno.h>

typedef struct ps_mutex_ops {
    void *cookie;
    void *(*mutex_new)(void);
    int (*mutex_lock)(void *m);
    int (*mutex_unlock)(void *m);
    int (*mutex_destroy)(void *m);
} ps_mutex_ops_t;

/**
 * Initialise a mutex
 *
 * @param ops Structure for the mutex operations.
 * @return A mutex handler on success, NULL on failure.
 */
static inline void *ps_mutex_new(ps_mutex_ops_t *ops)
{
    if (!ops || !ops->mutex_new) {
        ZF_LOGE("Argument passed to %s was NULL\n", __func__);
        return NULL;
    }
    return ops->mutex_new();
}

/**
 * Lock a mutex
 *
 * @param ops Structure for the mutex operations.
 * @param m Mutex handler.
 * @return 0 on success, an error code on failure.
 */
static inline int ps_mutex_lock(ps_mutex_ops_t *ops, void *m)
{
    if (!ops || !ops->mutex_lock) {
        ZF_LOGE("Argument passed to %s was NULL\n", __func__);
        return EINVAL;
    }
    return ops->mutex_lock(m);
}

/**
 * Unlock a mutex
 *
 * @param ops Structure for the mutex operations.
 * @param m Mutex handler.
 * @return 0 on success, an error code on failure.
 */
static inline int ps_mutex_unlock(ps_mutex_ops_t *ops, void *m)
{
    if (!ops || !ops->mutex_unlock) {
        ZF_LOGE("Argument passed to %s was NULL\n", __func__);
        return EINVAL;
    }
    return ops->mutex_unlock(m);
}

/**
 * Destroy a mutex
 *
 * @param ops Structure for the mutex operations.
 * @param m Mutex handler.
 * @return 0 on success, an error code on failure.
 */
static inline int ps_mutex_destroy(ps_mutex_ops_t *ops, void *m)
{
    if (!ops || !ops->mutex_destroy) {
        ZF_LOGE("Argument passed to %s was NULL\n", __func__);
        return EINVAL;
    }
    return ops->mutex_destroy(m);
}
