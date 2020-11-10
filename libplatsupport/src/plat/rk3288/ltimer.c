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

/* Implementation of a logical timer for rockpro64
 *
 * We use the 1 rk for timeouts and 1 for a millisecond tick.
 *
 * The configuration of timers that we use allows us to map only one page in.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    rk_t rk_timeout;
    rk_t rk_timestamp;
    ps_io_ops_t ops;
} rk_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    rk_ltimer_t *rk_ltimer = data;
    *time = rk_get_time(&rk_ltimer->rk_timestamp);
    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    rk_ltimer_t *rk_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = 0;
        int error = get_time(data, &current_time);
        assert(error == 0);
        ns -= current_time;
    }

    return rk_set_timeout(&rk_ltimer->rk_timeout, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    rk_ltimer_t *rk_ltimer = data;
    /* just reset the timeout timer */
    rk_stop(&rk_ltimer->rk_timeout);
    /* no need to start it again, that is done by
     * set_timeout automatically */
    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    rk_ltimer_t *rk_ltimer = data;
    /* NOTE: Note that timeout is primary, timestamp is secondary.
     *  this means that timeout holds the region mapping.
     *  We must first destroy timestamp before destroying timeout
     *  so that the region is freed last. */
    rk_destroy(&rk_ltimer->rk_timestamp);
    rk_destroy(&rk_ltimer->rk_timeout);
    ps_free(&rk_ltimer->ops.malloc_ops, sizeof(rk_ltimer_t), rk_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    /* mostly copied from dmt.c */
    int error;

    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    error = create_ltimer_simple(
                ltimer, ops, sizeof(rk_ltimer_t),
                get_time, set_timeout, reset, destroy
            );
    if (error) {
        ZF_LOGE("Failed to create ltimer for rk");
        return error;
    }

    rk_ltimer_t *rk_ltimer = ltimer->data;
    rk_ltimer->ops = ops;

    /* set up a timer for timeouts */
    rk_config_t rk_config = {
        .fdt_path = RK_TIMER_PATH,
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .user_cb_event = LTIMER_TIMEOUT_EVENT
    };

    error = rk_init(&rk_ltimer->rk_timeout, ops, rk_config);
    if (error) {
        ZF_LOGE("Failed to initialise timer (timeout)");
        destroy(rk_ltimer);
        return error;
    }

    /* no start for timeout timer */

    /* set up a timer for timestamps */
    rk_config.user_cb_event = LTIMER_OVERFLOW_EVENT;

    error = rk_init_secondary(&rk_ltimer->rk_timestamp, &rk_ltimer->rk_timeout, ops, rk_config);
    if (error) {
        ZF_LOGE("Failed to initialise timer (timestamp)");
        destroy(rk_ltimer);
        return error;
    }

    error = rk_start_timestamp_timer(&rk_ltimer->rk_timestamp);
    if (error) {
        ZF_LOGE("Failed to start timestamp timer");
        destroy(rk_ltimer);
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
