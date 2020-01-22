/*
 * Copyright 2020, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Minimal implementation of a logical timer for zynq
 *
 * Does not implement some functions yet.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>

#include <utils/util.h>

#include "../../ltimer.h"

/* Use ttc0_timer1 for timeouts/sleep */
#define TTC_TIMEOUT   TTC0_TIMER1
/* Use ttc1_timer1 to keep running for timestamp/gettime */
#define TTC_TIMESTAMP TTC1_TIMER1

#define N_TTCS 2
#define TIMEOUT_IDX 0
#define TIMESTAMP_IDX 1

typedef struct {
    ttc_t ttcs[N_TTCS];
    ps_io_ops_t ops;
    bool timeout_initialised;
    bool timestamp_initialised;
} ttc_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    ttc_ltimer_t *ttc_ltimer = data;
    *time = ttc_get_time(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time = 0;
        get_time(data, &time);
        if (ns <= time) {
            return ETIME;
        } else {
            ns -= time;
        }
    }

    return ttc_set_timeout(&ttc_ltimer->ttcs[TIMEOUT_IDX], ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    /* reset the timers */
    ttc_stop(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    ttc_start(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    ttc_stop(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    ttc_start(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    ttc_ltimer_t *ttc_ltimer = data;

    int error = 0;

    if (ttc_ltimer->timeout_initialised) {
        error = ttc_destroy(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
        ZF_LOGF_IF(error, "Failed to de-allocate the timeout timer");
    }

    if (ttc_ltimer->timestamp_initialised) {
        error = ttc_destroy(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
        ZF_LOGF_IF(error, "Failed to de-allocate the timestamp timer");
    }

    ps_free(&ttc_ltimer->ops.malloc_ops, sizeof(ttc_ltimer), ttc_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(ttc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    ttc_ltimer_t *ttc_ltimer = ltimer->data;
    ttc_ltimer->ops = ops;

    ttc_config_t config = {
        .io_ops = ops,
        .user_callback = callback,
        .user_callback_token = callback_token,
        .id = TTC_TIMEOUT,
    };

    ttc_config_t config_timestamp = {
        .io_ops = ops,
        .user_callback = callback,
        .user_callback_token = callback_token,
        .is_timestamp = true,
        .id = TTC_TIMESTAMP,
    };

    error = ttc_init(&ttc_ltimer->ttcs[TIMEOUT_IDX], config);
    if (error) {
        ZF_LOGE("Failed to init the timeout timer");
        ltimer_destroy(ltimer);
        return error;
    }

    ttc_ltimer->timeout_initialised = true;

    error = ttc_start(&ttc_ltimer->ttcs[TIMEOUT_IDX]);
    if (error) {
        ZF_LOGE("Failed to start the timeout timer");
        ltimer_destroy(ltimer);
        return error;
    }

    /* set the second ttc to be a timestamp counter */
    error = ttc_init(&ttc_ltimer->ttcs[TIMESTAMP_IDX], config_timestamp);
    if (error) {
        ZF_LOGE("Failed to init the timestamp timer");
        ltimer_destroy(ltimer);
        return error;
    }

    ttc_ltimer->timestamp_initialised = true;

    ttc_freerun(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    error = ttc_start(&ttc_ltimer->ttcs[TIMESTAMP_IDX]);
    if (error) {
        ZF_LOGE("Failed to start the timestamp timer");
        ltimer_destroy(ltimer);
        return error;
    }

    return 0;
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
