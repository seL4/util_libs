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
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/ltimer.h>
#include <platsupport/plat/rtc.h>
#include <platsupport/plat/dmt.h>
#include <platsupport/io.h>

#include "../../ltimer.h"

/*
 * We use two dm timers: one to keep track of an absolute time, the other for timeouts.
 */
/* hikey dualtimers have 2 timers per frame, we just use one from each */
typedef struct {
    dmt_t dmt_timeout;
    dmt_t dmt_timestamp;
    ps_io_ops_t ops;
} hikey_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    hikey_ltimer_t *hikey_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    *time = dmt_get_time(&hikey_ltimer->dmt_timestamp);
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

    hikey_ltimer_t *hikey_ltimer = data;
    return dmt_set_timeout(&hikey_ltimer->dmt_timeout, ns, type == TIMEOUT_PERIODIC, true);
}

static int reset(void *data)
{
    hikey_ltimer_t *hikey_ltimer = data;
    /* restart the rtc */
    dmt_stop(&hikey_ltimer->dmt_timeout);
    dmt_start(&hikey_ltimer->dmt_timeout);
    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    hikey_ltimer_t *hikey_ltimer = data;
    dmt_destroy(&hikey_ltimer->dmt_timeout);
    dmt_destroy(&hikey_ltimer->dmt_timestamp);
    ps_free(&hikey_ltimer->ops.malloc_ops, sizeof(hikey_ltimer_t), hikey_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    error = create_ltimer_simple(
                ltimer, ops, sizeof(hikey_ltimer_t),
                get_time, set_timeout, reset, destroy
            );
    if (error) {
        ZF_LOGE("Failed to create ltimer for hikey");
        return error;
    }

    hikey_ltimer_t *hikey_ltimer = ltimer->data;
    hikey_ltimer->ops = ops;

    /* set up a DMT for timeouts */
    dmt_config_t dmt_config = {
        .fdt_path = DMT_PATH,
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .user_cb_event = LTIMER_TIMEOUT_EVENT
    };

    error = dmt_init(&hikey_ltimer->dmt_timeout, ops, dmt_config);
    if (error) {
        ZF_LOGE("Failed to init dmt timeout timer");
        destroy(hikey_ltimer);
        return error;
    }

    error = dmt_start(&hikey_ltimer->dmt_timeout);
    if (error) {
        ZF_LOGE("Failed to start dmt timeout timer");
        destroy(hikey_ltimer);
        return error;
    }

    /* set up a DMT for timestamps */
    dmt_config.user_cb_event = LTIMER_OVERFLOW_EVENT;

    error = dmt_init_secondary(&hikey_ltimer->dmt_timestamp, &hikey_ltimer->dmt_timeout, ops, dmt_config);
    if (error) {
        ZF_LOGE("Failed to init dmt secondary for timestamps");
        destroy(hikey_ltimer);
        return error;
    }

    error = dmt_start(&hikey_ltimer->dmt_timestamp);
    if (error) {
        ZF_LOGE("Failed to start dmt timestamp timer");
        destroy(hikey_ltimer);
        return error;
    }

    error = dmt_set_timeout_ticks(&hikey_ltimer->dmt_timestamp, UINT32_MAX, true, true);
    if (error) {
        ZF_LOGE("Failed to set ticks for dmt timestamp timer");
        destroy(hikey_ltimer);
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
