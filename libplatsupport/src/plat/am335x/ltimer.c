/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Implementation of a logical timer for beagle bone
 *
 * We use the 1 DMT for timeouts and 1 for a millisecond tick.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <utils/frequency.h>

#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    dmt_t dmt_timeout;
    dmt_t dmt_timestamp;
    ps_io_ops_t ops;
} dmt_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    dmt_ltimer_t *dmt_ltimer = data;

    *time = dmt_get_time(&dmt_ltimer->dmt_timestamp);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    *resolution = NS_IN_MS;
    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = 0;
        int error = get_time(data, &current_time);
        assert(error == 0);
        if (ns < current_time) {
            return ETIME;
        }
        ns -= current_time;
    }

    return dmt_set_timeout(&dmt_ltimer->dmt_timeout, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;
    dmt_stop(&dmt_ltimer->dmt_timeout);
    dmt_stop(&dmt_ltimer->dmt_timestamp);
    dmt_start(&dmt_ltimer->dmt_timeout);
    dmt_start(&dmt_ltimer->dmt_timestamp);
    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;
    dmt_destroy(&dmt_ltimer->dmt_timeout);
    dmt_destroy(&dmt_ltimer->dmt_timestamp);
    ps_free(&dmt_ltimer->ops.malloc_ops, sizeof(dmt_ltimer_t), dmt_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    error = create_ltimer_simple(
                ltimer, ops, sizeof(dmt_ltimer_t),
                get_time, set_timeout, reset, destroy
            );
    if (error) {
        ZF_LOGE("Failed to create ltimer");
        return error;
    }
    ltimer->get_resolution = get_resolution;

    dmt_ltimer_t *dmt_ltimer = ltimer->data;
    dmt_ltimer->ops = ops;

    dmt_config_t dmt_config = {
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .user_cb_event = LTIMER_OVERFLOW_EVENT,
        .fdt_path = DMTIMER2_PATH,
    };

    error = dmt_init(&dmt_ltimer->dmt_timestamp, ops, dmt_config);
    if (error) {
        ZF_LOGE("Failed to init dmt (TIMEKEEPING)");
        destroy(dmt_ltimer);
        return error;
    }

    error = dmt_start_ticking_timer(&dmt_ltimer->dmt_timestamp);
    if (error) {
        ZF_LOGE("Failed to start dmt (TIMEKEEPING)");
        destroy(dmt_ltimer);
        return error;
    }

    dmt_config.fdt_path = DMTIMER3_PATH;
    dmt_config.user_cb_event = LTIMER_TIMEOUT_EVENT;

    error = dmt_init(&dmt_ltimer->dmt_timeout, ops, dmt_config);
    if (error) {
        ZF_LOGE("Failed to init dmt (TIMEOUT)");
        destroy(dmt_ltimer);
        return error;
    }

    error = dmt_start(&dmt_ltimer->dmt_timeout);
    if (error) {
        ZF_LOGE("Failed to start dmt (TIMEOUT)");
        destroy(dmt_ltimer);
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
