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

/**
 * This file provides a time manager for managing time and timeouts.
 * It is intended to be used to multiplex timeouts to a single timeout
 * for use in time servers.
 *
 * Implementations of this interface should be reentrant: it should be
 * valid to call callbacks from tm_update_with_time.
 *
 * Whether or not the implementation is thread safe is implementation specific and
 * not mandated by the interface.
 */
#include <stdint.h>
#include <errno.h>
#include <platsupport/ltimer.h>

/* function to call when a timeout comes in */
typedef int (*timeout_cb_fn_t)(uintptr_t token);

typedef struct time_manager {
    /*
     * Obtain a new id to use to register callbacks with.
     *
     * @param data data specific to this implementation.
     * @param id   memory to store the allocated id in.
     * @return 0 on success, EINVAL if data or id is invalid, ENOMEM if no ids are available.
     */
     int (*alloc_id)(void *data, unsigned int *id);

    /*
     * Allocate a specific id to register callbacks with.
     *
     * @param data data specific to this implementation.
     * @param id   specific id to use.
     * @return 0 on success, EINVAL if data or id is invalid, EADDRINUSE if the id
     *              is already in use.
     */
     int (*alloc_id_at)(void *data, unsigned int id);

    /*
     * Inform the timer manager that this id is free and no longer going to be used, which
     * means the id can be handed out by tm_new_id.
     *
     * @param data data specific to this implementation
     * @param id   id allocated by tm_new_id to be free'd.
     */
     int (*free_id)(void *data, unsigned int id);

    /*
     * Register a callback to call when a specific timeout id fires. The implementation does not spin and
     * other threads may run. The callback will be called on the stack of the thread that calls tm_update.
     * or tm_register_cb.
     *
     * The callback is allowed to re- or de-register itself.
     *
     * Only one callback can be registered per id. If a callback is already registered for this specific id
     * it will be overridden by this function call.
     *
     * @param data data specific to this implementation
     * @param oneshot_ns amount of nanoseconds to wait.
     * @param start      timestamp to first call the callback. If value is 0 or already passed.
     *                   the callback will be called ns time after this function is called.
     * @param id         id obtained from tm_new_id. If this id already exists the callback will be updated.
     * @param callback   the callback to call when the timeout(s) occur.
     * @param token      token to pass to the callback function.
     * @return           0 on success, errno on error.
     */
     int (*register_cb)(void *data, timeout_type_t type, uint64_t ns,
                     uint64_t start, uint32_t id, timeout_cb_fn_t callback, uintptr_t token);

    /*
     * Turn off a callback. The callback will not be called unless it is registered again, however the
     * id cannot be reused until tm_free_id is called.
     *
     * @param data data specific to this implementation
     * @param id   id of the callback. If this id already exists the callback will be updated.
     * @return     0 on success, EINVAL if data or id are invalid.
     */
     int (*deregister_cb)(void *data, uint32_t id);

      /*
       * Signal to the timer manager to check if any callbacks are due to be called,
       * based on the passed in valid for time.
       *
       * @param data data specific to this implementation
       * @param time  the current time.
       *
       * @return 0 on success, EINVAL if data is invalid.
       */
      int (*update_with_time)(void *data, uint64_t time);

      /*
       * Get the current time in nanoseconds.
       *
       * @param data    data specific to this implementation
       * @param[out]    time memory to return the time in nanoseconds
       * @return        0 on success, EINVAL id data or time are invalid.
       */
      int (*get_time)(void *data, uint64_t *time);

      /* data specific to this implementation and passed to all functions */
      void *data;

} time_manager_t;

#define __TM_VALID_ARGS(FUN) do {\
    if (!tm) return EINVAL;\
    if (!tm->FUN) return ENOSYS;\
} while (0)

/* helper functions */
static inline int tm_alloc_id(time_manager_t *tm, unsigned int *id)
{
    __TM_VALID_ARGS(alloc_id);

    if (!id) {
        return EINVAL;
    }

    return tm->alloc_id(tm->data, id);
}

static inline int tm_alloc_id_at(time_manager_t *tm, unsigned int id)
{
    __TM_VALID_ARGS(alloc_id_at);
    return tm->alloc_id_at(tm->data, id);
}

static inline int tm_free_id(time_manager_t *tm, unsigned int id)
{
    __TM_VALID_ARGS(free_id);
    return tm->free_id(tm->data, id);
}


static inline int tm_register_cb(time_manager_t *tm, timeout_type_t type, uint64_t ns,
                                 uint64_t start, uint32_t id, timeout_cb_fn_t callback, uintptr_t token) {
    __TM_VALID_ARGS(register_cb);
    return tm->register_cb(tm->data, type, ns, start, id, callback, token);
}
/*
 * Call a callback after a specific time. The implementation does not spin and
 * other threads may run. Callbacks will not be called until tm_update is called.
 *
 * @param tm         the timer manager.
 * @param abs_ns     time to call the callback
 * @param id         id obtained from tm_new_id. If this id already exists the callback will be updated.
 * @param callback   the callback to call when the timeout(s) occur.
 * @param token      token to pass to the callback function.
 * @return         0 on success, errno on error.
 */
static inline int tm_register_abs_cb(time_manager_t *tm, uint64_t abs_ns, uint32_t id,
                                                timeout_cb_fn_t callback, uintptr_t token)
{
    __TM_VALID_ARGS(register_cb);
    return tm->register_cb(tm->data, TIMEOUT_ABSOLUTE, abs_ns, 0, id, callback, token);
}

/*
 * Call a callback after ns nanoseconds. The implementation does not spin and
 * other threads may run.
 *
 * @param tm         the timer manager.
 * @param rel_ns amount of nanoseconds to wait
 * @param id         id of the callback. If this id already exists the callback will be updated.
 * @param callback   the callback to call when the timeout(s) occur.
 * @param token      token to pass to the callback function.
 * @return           0 on success, errno on error.
 */
static inline int tm_register_rel_cb(time_manager_t *tm, uint64_t rel_ns, uint32_t id,
                                                timeout_cb_fn_t callback, uintptr_t token)
{
    __TM_VALID_ARGS(register_cb);
    return tm->register_cb(tm->data, TIMEOUT_RELATIVE, rel_ns, 0, id, callback, token);
}

/*
 * Call a callback every ns nanoseconds. The implementation does not spin and
 * other threads may run.
 *
 * @param tm         the timer manager.
 * @param period_ns  call the callback everytime period_ns expires, from start
 * @param start      timestamp to first call the callback. If value is 0 or already passed
 *                   the callback will be called period_ns time after this function is called.
 * @param id         id of the callback. If this id already exists the callback will be updated.
 * @param callback   the callback to call when the timeout(s) occur.
 * @param token      token to pass to the callback function.
 * @return           0 on success, errno on error.
 */
static inline int tm_register_periodic_cb(time_manager_t *tm, uint64_t period_ns, uint64_t start,
                                                uint32_t id, timeout_cb_fn_t callback, uintptr_t token)
{
    __TM_VALID_ARGS(register_cb);
    return tm->register_cb(tm->data, TIMEOUT_PERIODIC, period_ns, start, id, callback, token);
}

static inline int tm_deregister_cb(time_manager_t *tm, unsigned int id)
{
    __TM_VALID_ARGS(deregister_cb);
    return tm->deregister_cb(tm->data, id);
}

static inline int tm_get_time(time_manager_t *tm, uint64_t *time)
{
    __TM_VALID_ARGS(get_time);
    if (!time) {
        return EINVAL;
    }
    return tm->get_time(tm->data, time);
}

/*
 * As per update_with_time, but get the current time first.
 */
static inline int tm_update(time_manager_t *tm)
{
    __TM_VALID_ARGS(update_with_time);
    uint64_t time;
    int error = tm_get_time(tm, &time);
    if (error) {
        return error;
    }

    return tm->update_with_time(tm->data, time);
}

static inline int tm_update_with_time(time_manager_t *tm, uint64_t time)
{
    __TM_VALID_ARGS(update_with_time);
    return tm->update_with_time(tm->data, time);
}
