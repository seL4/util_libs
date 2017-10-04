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

/**
 * This file provides a timer manager for managing multiple timeouts.
 * It is intended to be used to multiplex timeouts to a single timeout
 * for use in time servers.
 */
#include <platsupport/time_manager.h>
#include <platsupport/local_time_manager.h>
#include <platsupport/tqueue.h>
#include <platsupport/ltimer.h>


typedef struct time_man_state {
    ltimer_t *ltimer;
    tqueue_t timeouts;
    uint64_t current_timeout;
} time_man_state_t;

static int alloc_id(void *data, unsigned int *id)
{
    time_man_state_t *state = data;
    return tqueue_alloc_id(&state->timeouts, id);
}

static int alloc_id_at(void *data, unsigned int id)
{
    time_man_state_t *state = data;
    return tqueue_alloc_id_at(&state->timeouts, id);
}

static int free_id(void *data, unsigned int id)
{
    time_man_state_t *state = data;
    return tqueue_free_id(&state->timeouts, id);
}

static int get_time(void *data, uint64_t *time)
{
    assert(data && time);
    time_man_state_t *state = data;

    /* get the time */
    return ltimer_get_time(state->ltimer, time);
}

static int update_with_time(void *data, uint64_t curr_time)
{
    uint64_t next_time;
    int error = 0;

    time_man_state_t *state = data;
    do {
        error = tqueue_update(&state->timeouts, curr_time, &next_time);
        if (error) {
            ZF_LOGE("timeout update failed");
            return error;
        }

        if (next_time == 0) {
            /* nothing to set */
            state->current_timeout = UINT64_MAX;
        } else if (next_time < state->current_timeout || state->current_timeout < curr_time) {
            state->current_timeout = next_time;
            error = ltimer_set_timeout(state->ltimer, next_time, TIMEOUT_ABSOLUTE);
            if (error == ETIME) {
                error = ltimer_get_time(state->ltimer, &curr_time);
            }
        }

    } while (error == ETIME || error != 0);

    return error;
}

static int register_cb(void *data, timeout_type_t type, uint64_t ns,
                       uint64_t start, uint32_t id, timeout_cb_fn_t callback, uintptr_t token)
{
    time_man_state_t *state = data;
    timeout_t timeout = {0};
    uint64_t curr_time;

    int error = get_time(data, &curr_time);
    if (error) {
        return error;
    }

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        timeout.abs_time = ns;
        break;
    case TIMEOUT_RELATIVE:
        timeout.abs_time = curr_time + ns;
        break;
    case TIMEOUT_PERIODIC:
        if (start) {
            timeout.abs_time = start;
        } else {
            timeout.abs_time = curr_time + ns;
        }
        timeout.period = ns;
        break;
    default:
        return EINVAL;
    }

    if (timeout.abs_time < curr_time) {
        return ETIME;
    }

    timeout.token = token;
    timeout.callback = callback;
    error = tqueue_register(&state->timeouts, id, &timeout);
    if (error) {
        return error;
    }

    /* if its within a microsecond, don't bother to reset the timeout to avoid races */
    if (timeout.abs_time + NS_IN_US < state->current_timeout) {
        error = ltimer_set_timeout(state->ltimer, timeout.abs_time, TIMEOUT_ABSOLUTE);
        if (error == ETIME) {
            /* set it to slightly more than current time as we raced */
            uint64_t backup_timeout = curr_time + 10 * NS_IN_US;
            error = ltimer_set_timeout(state->ltimer, backup_timeout, TIMEOUT_ABSOLUTE);
            if (error == ETIME) {
               ZF_LOGF_IF(error, "Failed to set timeout in 10 us. Timeout not set.");
            } else if (error) {
                ZF_LOGF("should not be possible.");
            }
            state->current_timeout = backup_timeout;
        } else if (error == 0) {
            state->current_timeout = timeout.abs_time;
        }
    }
    return error;
}

static int deregister_cb(void *data, uint32_t id)
{
    /* we don't cancel the irq on the ltimer here, as checking if we updated the head
     * of the queue and resetting a timeout are probably comparable to
     * getting an extra irq */
    time_man_state_t *state = data;
    return tqueue_cancel(&state->timeouts, id);
}

int tm_init(time_manager_t *tm, ltimer_t *ltimer, ps_io_ops_t *ops, int size) {

    if (!tm || !ltimer) {
        return EINVAL;
    }

    tm->alloc_id = alloc_id;
    tm->free_id = free_id;
    tm->alloc_id_at = alloc_id_at;
    tm->register_cb = register_cb;
    tm->deregister_cb = deregister_cb;
    tm->update_with_time = update_with_time;
    tm->get_time = get_time;

    int error = ps_calloc(&ops->malloc_ops, 1, sizeof(time_man_state_t), &tm->data);
    if (error) {
        return error;
    }

    time_man_state_t *state = tm->data;
    state->ltimer = ltimer;
    state->current_timeout = UINT64_MAX;
    error = tqueue_init_static(&state->timeouts, &ops->malloc_ops, size);

    if (error) {
        ps_free(&ops->malloc_ops, sizeof(time_man_state_t), &tm->data);
    }
    return error;
}
