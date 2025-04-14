/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/ltimer.h>
#include <platsupport/plat/sp804.h>
#include <platsupport/io.h>

#include "../../ltimer.h"

/*
 * We use two sp804 timers: one to keep track of an absolute time, the other for timeouts.
 */

typedef struct {
    /* fvp sp804 have 2 timers per frame, we just use one per each */
    sp804_t sp804_timeout;
    sp804_t sp804_timestamp;
    ps_io_ops_t ops;
} fvp_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    fvp_ltimer_t *fvp_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    *time = sp804_get_time(&fvp_ltimer->sp804_timestamp);
    return 0;
}

int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time;
        int error = get_time(data, &time);
        if (error) {
            return error;
        }
        if (time > ns) {
            return ETIME;
        }
        ns -= time;
    }

    fvp_ltimer_t *fvp_ltimer = data;
    return sp804_set_timeout(&fvp_ltimer->sp804_timeout, ns, type == TIMEOUT_PERIODIC, true);
}

static int reset(void *data)
{
    fvp_ltimer_t *fvp_ltimer = data;
    /* restart the rtc */
    sp804_stop(&fvp_ltimer->sp804_timeout);
    sp804_start(&fvp_ltimer->sp804_timeout);
    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    fvp_ltimer_t *fvp_ltimer = data;
    sp804_destroy(&fvp_ltimer->sp804_timeout);
    sp804_destroy(&fvp_ltimer->sp804_timestamp);
    ps_free(&fvp_ltimer->ops.malloc_ops, sizeof(fvp_ltimer_t), fvp_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    error = create_ltimer_simple(
                ltimer, ops, sizeof(fvp_ltimer_t),
                get_time, set_timeout, reset, destroy
            );
    if (error) {
        ZF_LOGE("Failed to create ltimer simple");
        return error;
    }

    fvp_ltimer_t *fvp_ltimer = ltimer->data;
    fvp_ltimer->ops = ops;

    /* set up an SP804 for timeouts */
    sp804_config_t sp804_config = {
        .fdt_path = SP804_TIMER1_PATH,
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .user_cb_event = LTIMER_TIMEOUT_EVENT
    };

    error = sp804_init(&fvp_ltimer->sp804_timeout, ops, sp804_config);
    if (error) {
        ZF_LOGE("Failed to init timeout timer");
        destroy(&fvp_ltimer);
        return error;
    }

    error = sp804_start(&fvp_ltimer->sp804_timeout);
    if (error) {
        ZF_LOGE("Failed to start timeout timer");
        destroy(&fvp_ltimer);
        return error;
    }

    /* another for timestamps */
    sp804_config.fdt_path = SP804_TIMER2_PATH;
    sp804_config.user_cb_event = LTIMER_OVERFLOW_EVENT;

    error = sp804_init(&fvp_ltimer->sp804_timestamp, ops, sp804_config);
    if (error) {
        ZF_LOGE("Failed to init timestamp timer");
        destroy(&fvp_ltimer);
        return error;
    }

    error = sp804_start(&fvp_ltimer->sp804_timestamp);
    if (error) {
        ZF_LOGE("Failed to start timestamp timer");
        destroy(&fvp_ltimer);
        return error;
    }

    error = sp804_set_timeout_ticks(&fvp_ltimer->sp804_timestamp, UINT32_MAX, true, true);
    if (error) {
        ZF_LOGE("Failed to set timeout ticks for timer");
        destroy(&fvp_ltimer);
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
